/* ch11-canary/fortify/demo.c
 *
 * Full source of Chapter 11's lst-fortify listing. The three functions are
 * the three cases FORTIFY_SOURCE cares about, at levels 1, 2, and 3:
 *
 *   copy_static  : destination size known at compile time -- every level
 *                  rewrites strcpy -> __strcpy_chk (positive control).
 *   copy_dynamic : destination size known only at runtime (from malloc(n))
 *                  -- levels 1 and 2 CANNOT reason about this and emit the
 *                  raw strcpy; level 3 uses __builtin_dynamic_object_size
 *                  to trace back to the alloc and rewrites to __strcpy_chk
 *                  with n as the bound.
 *   copy_manual  : hand-rolled loop, no libc sink -- FORTIFY has nothing to
 *                  wrap and cannot help.
 *
 * The Makefile in this directory builds demo three times (F_S=1, 2, 3) and
 * disassembles copy_dynamic so exercise 5 has ready-made evidence.
 *
 * The functions must be reachable from main() so gcc does not DCE them.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* noinline keeps copy_* out of main() so `objdump --disassemble=copy_*`
 * has a function to look at, regardless of -O2 inlining decisions. */
#define NOINLINE __attribute__((noinline, noclone))

/* Case A: destination size is obvious at compile time.
   FORTIFY at any level rewrites strcpy -> __strcpy_chk with dstlen = 64. */
NOINLINE
void copy_static(const char *src)
{
    char buf[64];                                  // (1) size known statically
    strcpy(buf, src);                              // becomes __strcpy_chk(buf, src, 64)
    puts(buf);
}

/* Case B: destination size is only known at runtime.
   FORTIFY=1/=2 CANNOT reason about this; FORTIFY=3 CAN, via
   __builtin_dynamic_object_size, and rewrites strcpy -> __strcpy_chk
   with the runtime allocation size as the bound. */
NOINLINE
void copy_dynamic(size_t n, const char *src)
{
    char *buf = malloc(n);                         // (2) size known at runtime
    strcpy(buf, src);                              // =3 becomes __strcpy_chk(buf, src, n)
    puts(buf);
    free(buf);
}

/* Case C: sink is not a fortified function. FORTIFY cannot help you here
   because it does not touch memcpy loops the compiler cannot pattern-match. */
NOINLINE
void copy_manual(char *dst, const char *src, size_t n)
{
    for (size_t i = 0; i < n; i++)                 // (3) hand-rolled: no wrapper
        dst[i] = src[i];
}

/* Keep each case reachable at some optimization levels; the Makefile only
 * inspects disassembly of copy_dynamic, but this keeps the other two live. */
int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <A|B|C> <string> [n]\n", argv[0]);
        return 2;
    }
    switch (argv[1][0]) {
    case 'A': copy_static(argv[2]); break;
    case 'B': {
        size_t n = (argc > 3) ? (size_t)strtoul(argv[3], NULL, 0) : 64;
        copy_dynamic(n, argv[2]);
        break;
    }
    case 'C': {
        size_t n = (argc > 3) ? (size_t)strtoul(argv[3], NULL, 0) : strlen(argv[2]) + 1;
        char *d = malloc(n);
        copy_manual(d, argv[2], n);
        puts(d);
        free(d);
        break;
    }
    default: return 2;
    }
    return 0;
}
