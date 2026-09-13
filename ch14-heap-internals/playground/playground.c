/* build: gcc -O0 -g playground.c -o playground   (glibc 2.39, Ubuntu 24.04)
 *
 * Instrumented allocator playground. Lab use only.
 *
 * Commands read one per line from stdin (# starts a comment, blank lines
 * ignored). Every command echoes what it did so scripted runs diff cleanly
 * against a golden file.
 *
 *   alloc N        allocate a chunk of N bytes, print its index and user ptr
 *   free  i        free chunk at index i (does not clear the slot)
 *   dump  i        dump chunk header + flags + neighbor P-bit (see book Fig 14-1)
 *   tcache         print the tcache_perthread_struct counts array
 *   bins           alias of tcache (small/large bin walk needs libc internals;
 *                  use pwndbg for those --- see ch14-heap-internals/pwndbg/)
 *   leak  i        print chunk[i]'s raw first 8 bytes (mangled fd) and the
 *                  safe-linking REVEAL against the chunk's own address
 *   quit           exit 0
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- metadata dumper (mirrors Listing 14-5 in the book) ---------------- */
#define CHUNK_HDR(p)  ((uintptr_t*)((char*)(p) - 16))            // ❶
#define SIZE_BITS     0x7UL
#define PREV_INUSE    0x1UL
#define IS_MMAPPED    0x2UL
#define NON_MAIN_ARENA 0x4UL

void chunk_dump(void *p) {
    uintptr_t *h = CHUNK_HDR(p);
    uintptr_t prev = h[0], sz = h[1];                            // ❷
    uintptr_t real = sz & ~SIZE_BITS;
    printf("user=%p  hdr=%p\n", p, (void*)h);
    printf("  prev_size = 0x%lx\n", prev);
    printf("  size      = 0x%lx  (real 0x%lx, flags:%s%s%s )\n",
           sz, real,
           (sz & PREV_INUSE)     ? " P" : "",                    // ❸
           (sz & IS_MMAPPED)     ? " M" : "",
           (sz & NON_MAIN_ARENA) ? " A" : "");
    uintptr_t *next = (uintptr_t*)((char*)h + real);             // ❹
    printf("  next.size = 0x%lx  (its P bit reflects US: %s)\n",
           next[1], (next[1] & PREV_INUSE) ? "in-use" : "FREE");
}

/* ---- slot table -------------------------------------------------------- */
#define NSLOTS 64
static void  *slots[NSLOTS];
static size_t sizes[NSLOTS];

static int alloc_cmd(size_t n) {
    for (int i = 0; i < NSLOTS; i++) {
        if (!slots[i]) {
            slots[i] = malloc(n);
            sizes[i] = n;
            if (!slots[i]) { printf("alloc: OOM\n"); return -1; }
            /* Wipe so leak/dump are deterministic before any user write.
             * (glibc will overwrite the first 16 bytes on free anyway.)  */
            memset(slots[i], 0, n);
            printf("alloc[%d] size=0x%zx user=%p\n", i, n, slots[i]);
            return i;
        }
    }
    printf("alloc: slot table full\n");
    return -1;
}

static void free_cmd(int i) {
    if (i < 0 || i >= NSLOTS || !slots[i]) { printf("free[%d]: empty\n", i); return; }
    printf("free[%d] user=%p size=0x%zx\n", i, slots[i], sizes[i]);
    free(slots[i]);
    slots[i] = NULL;   /* forget the dangling ptr; use dump-before-free */
    sizes[i] = 0;
}

/* We can still dump a freed chunk if the user kept the index: to allow that
 * the playground remembers the last-freed pointer under a shadow table. */
static void *shadow[NSLOTS];
static size_t shadow_sz[NSLOTS];

static int alloc_cmd_shadow(size_t n) {
    int i = alloc_cmd(n);
    if (i >= 0) { shadow[i] = slots[i]; shadow_sz[i] = n; }
    return i;
}

static void free_cmd_shadow(int i) {
    if (i < 0 || i >= NSLOTS || !slots[i]) { printf("free[%d]: empty\n", i); return; }
    printf("free[%d] user=%p size=0x%zx\n", i, slots[i], sizes[i]);
    free(slots[i]);
    slots[i] = NULL;
    sizes[i] = 0;
    /* shadow[i] intentionally retained --- dump/leak on a freed slot works */
}

static void dump_cmd(int i) {
    void *p = shadow[i] ? shadow[i] : NULL;
    if (i < 0 || i >= NSLOTS || !p) { printf("dump[%d]: no such slot\n", i); return; }
    printf("dump[%d] (allocated=%s):\n", i, slots[i] ? "yes" : "no");
    chunk_dump(p);
}

static void leak_cmd(int i) {
    void *p = shadow[i] ? shadow[i] : NULL;
    if (i < 0 || i >= NSLOTS || !p) { printf("leak[%d]: no such slot\n", i); return; }
    uintptr_t raw = *(uintptr_t*)p;
    uintptr_t reveal = raw ^ ((uintptr_t)p >> 12);
    printf("leak[%d] user=%p raw_fd=0x%016lx reveal=0x%016lx\n",
           i, p, raw, reveal);
}

/* tcache_perthread_struct is the FIRST allocated chunk on any heap. Its
 * user area begins with uint16_t counts[64] followed by tcache_entry*
 * entries[64]. We recover it by walking backwards from any live chunk to
 * page start and locating the 0x291-sized allocated chunk.               */
static void tcache_cmd(void) {
    /* Force at least one allocation so tcache exists. */
    void *probe = malloc(0x18);
    /* tcache header chunk sits just before the first user chunk on the
     * arena. Its header size is 0x291 on 64-bit (0x290 + PREV_INUSE).   */
    uintptr_t page = ((uintptr_t)probe) & ~0xfffUL;
    uintptr_t scan = page;
    uintptr_t *tc_user = NULL;
    for (int k = 0; k < 0x1000/8; k++) {
        uintptr_t *h = (uintptr_t*)(scan + k*8);
        if (h[1] == 0x291) { tc_user = h + 2; break; }
    }
    free(probe);
    if (!tc_user) { printf("tcache: header not found on first page\n"); return; }
    uint16_t *counts = (uint16_t*)tc_user;
    printf("tcache counts (non-zero bins):\n");
    int any = 0;
    for (int b = 0; b < 64; b++) {
        if (counts[b]) {
            size_t chunk_sz = 0x20 + b*0x10;  /* tcache bin size in bytes */
            printf("  bin[%2d] chunk_sz=0x%zx  count=%u\n", b, chunk_sz, counts[b]);
            any = 1;
        }
    }
    if (!any) printf("  (all bins empty)\n");
}

/* ---- command loop ------------------------------------------------------ */
int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        /* strip trailing newline */
        size_t L = strlen(line);
        while (L && (line[L-1]=='\n' || line[L-1]=='\r')) line[--L] = 0;
        if (!L || line[0] == '#') continue;
        char cmd[16]; long arg = 0;
        int nf = sscanf(line, "%15s %ld", cmd, &arg);
        if (nf < 1) continue;
        if      (!strcmp(cmd, "alloc"))  alloc_cmd_shadow((size_t)arg);
        else if (!strcmp(cmd, "free"))   free_cmd_shadow((int)arg);
        else if (!strcmp(cmd, "dump"))   dump_cmd((int)arg);
        else if (!strcmp(cmd, "leak"))   leak_cmd((int)arg);
        else if (!strcmp(cmd, "tcache")) tcache_cmd();
        else if (!strcmp(cmd, "bins"))   tcache_cmd();
        else if (!strcmp(cmd, "quit"))   { printf("bye\n"); break; }
        else printf("?unknown: %s\n", cmd);
    }
    (void)argc; (void)argv;
    return 0;
}
