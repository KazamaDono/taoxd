/* ch32-symbolic/src/parser.c
 *
 * Length-prefixed record parser with a classic signed/unsigned confusion
 * on the record length.  A negative `len` field survives the signed
 * bounds check but is cast to a huge `size_t` inside memcpy(), giving
 * an out-of-bounds copy.  Target for angr's find=crash-addr flow.
 *
 * Wire format read from stdin:
 *   offset  size  meaning
 *   0       4     magic  "PKT\0"
 *   4       4     len    (int32_t, little-endian; MUST be < 32 signed)
 *   8       len   payload (as size_t, so a negative len is enormous)
 *
 * Lab use only.  INSECURE ON PURPOSE.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INBUF  64
#define DSTBUF 32

/* Publicly visible so angr can resolve it via
 *     p.loader.find_symbol('do_copy').rebased_addr
 * (see solve/find_crash.py).  This is the site the solver reaches. */
__attribute__((noinline)) void do_copy(char *dst, const char *src, size_t n)
{
    /* Vulnerable: `n` is size_t so a negative int32_t `len` cast up is huge. */
    memcpy(dst, src, n);
}

int main(void)
{
    unsigned char buf[INBUF];
    char dst[DSTBUF];

    ssize_t r = read(0, buf, INBUF);
    if (r < 8) return 1;

    if (memcmp(buf, "PKT", 4) != 0) return 1;

    int32_t len;
    memcpy(&len, buf + 4, 4);

    /* SIGNED comparison — a negative len passes trivially. */
    if (len >= (int32_t)DSTBUF) return 1;

    /* But the copy uses size_t, so negative len -> huge unsigned. */
    do_copy(dst, (const char *)buf + 8, (size_t)len);

    /* Consume dst so the compiler cannot delete it. */
    if (dst[0] == 'z') puts("z");
    return 0;
}
