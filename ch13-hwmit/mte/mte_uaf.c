/* Build: clang-18 --target=aarch64-linux-gnu -O1 -march=armv8.5-a+memtag \
 *          -fsanitize=memtag-stack -fsanitize=memtag-heap -fuse-ld=lld \
 *          mte_uaf.c -o mte_uaf
 * Run:   qemu-aarch64 -cpu max ./mte_uaf
 *
 * Use-after-free caught by the MTE-aware allocator's retag-on-free (scudo
 * on bionic; glibc-2.35+ ptmalloc with mtag hooks otherwise): the freed
 * chunk's granules are re-tagged when returned to the pool, so the stale
 * pointer carries the old tag and dereferencing it faults with SEGV_MTESERR.
 */
#define _GNU_SOURCE
#include <sys/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifndef PR_MTE_TCF_SYNC
#define PR_MTE_TCF_SYNC (1UL << 1)
#define PR_SET_TAGGED_ADDR_CTRL 55
#endif
#ifndef SEGV_MTESERR
#define SEGV_MTESERR 9
#endif

static void on_sigsegv(int s, siginfo_t *si, void *uc) {
    dprintf(2, "SIGSEGV @ %p  si_code=%d  (%s)\n",
        si->si_addr, si->si_code,
        si->si_code == SEGV_MTESERR ? "MTE tag mismatch (UAF)" : "other");
    _exit(1);
}

int main(void) {
    struct sigaction sa = { .sa_flags = SA_SIGINFO, .sa_sigaction = on_sigsegv };
    sigemptyset(&sa.sa_mask); sigaction(SIGSEGV, &sa, NULL);

    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
              PR_MTE_TCF_SYNC | (0xfffe << 3), 0, 0, 0) < 0) {
        perror("prctl(MTE)"); return 2;
    }

    char *stale = malloc(32);
    memset(stale, 'A', 32);
    printf("alloc  stale = %p (tag in top byte)\n", stale);
    free(stale);                       /* scudo retags the granules here */

    char *fresh = malloc(32);          /* likely lands in the same slot */
    printf("realloc fresh = %p\n", fresh);

    /* Dereference the stale pointer: its tag no longer matches memory. */
    volatile char c = stale[0];
    (void)c;
    puts("unreachable");
    return 0;
}
