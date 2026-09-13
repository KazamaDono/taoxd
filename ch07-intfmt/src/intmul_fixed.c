/* ch07-intfmt/src/intmul_fixed.c -- remediated intmul.c.  LAB (defensive) demo.
 *
 * The chapter prints only the load-bearing excerpt (the ckd_mul call); this is
 * the full, runnable source.  It is byte-for-byte intmul.c except that the size
 * calculation is done with a C23 checked multiply that refuses to produce a
 * wrapped value, so the exploitation chain in solve_intmul.py dies at its root.
 *
 * Portability note: <stdckdint.h> (and the ckd_mul macro) is C23, shipped by
 * clang 18 and gcc 14+.  gcc 13 -- the Ubuntu 24.04 default -- predates the
 * header, so we fall back to __builtin_mul_overflow, which ckd_mul merely wraps.
 * Either way the semantics are identical and the overflow is rejected. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__has_include) && __has_include(<stdckdint.h>)
#  include <stdckdint.h>          /* C23; gcc 14+ / clang 18 */
#else
#  define ckd_mul(r, a, b) __builtin_mul_overflow((a), (b), (r))
#endif

struct rec  { char data[32]; };                 /* 32-byte records        */
struct auth { long is_admin; char user[24]; };  /* the adjacent victim    */

static uint32_t rd_u32(void)
{
    uint32_t v = 0;
    if (fread(&v, 1, sizeof v, stdin) != sizeof v) exit(1);
    return v;
}

int main(void)
{
    uint32_t count  = rd_u32();                            /* attacker */
    uint32_t nbytes = rd_u32();                            /* attacker */
    (void) rd_u32();                                       /* padding  */

    uint32_t size;
    if (ckd_mul(&size, count, (uint32_t) sizeof(struct rec))) {  /* ❶ */
        fputs("size overflow rejected\n", stderr);
        return 1;                 /* ❷ refuse to allocate a wrong size */
    }
    struct rec  *recs = malloc(size);
    struct auth *a    = calloc(1, sizeof *a);
    if (!recs || !a) return 1;

    fread(recs, 1, nbytes, stdin);

    if (a->is_admin) puts("ACCESS GRANTED");
    else             puts("access denied");
    return 0;
}
