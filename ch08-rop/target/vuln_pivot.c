/* ch08-rop/target/vuln_pivot.c - dedicated lab target for the stack-pivot demo.
 * Lab use only. See ../../ETHICS.md.
 * Built NX-on, canary-off, PIE-off (see Makefile) — same teaching choice as
 * vuln.c. The difference: the overflow here is *short*, so the classic chain
 * does not fit after the return address and you must pivot rsp into g_buf.
 *
 * g_buf is a large global in BSS. Because the binary is -no-pie its address is
 * fixed, so the exploit can stage a full ROP chain there and then pivot to it
 * with `leave ; ret` (see ../exploit/pivot_demo.py, Listing lst-pivot).
 *
 * It is deliberately large: after the pivot, rsp lives *inside* g_buf, so any
 * libc function the chain calls (puts, read, system) grows its own frame
 * DOWNWARD from the pivot point. The exploit pivots to slots high in g_buf (see
 * STAGE1_OFF / STAGE2_OFF there) so that downward growth stays inside this
 * buffer instead of walking down into the GOT/.data below it and faulting. */
#include <stdio.h>
#include <unistd.h>

/* 16-byte aligned so the pivoted rsp has predictable alignment for movaps.
 * Sized to hold the staged chain AND leave several KB of headroom below the
 * pivot point for the libc frames the chain calls into. */
#define GBUF_SZ 0x2000
char g_buf[GBUF_SZ] __attribute__((aligned(16)));   /* fixed-address staging area */

static void vuln(void)
{
    char buf[64];                      /* 64-byte buffer: saved rbp @64, ret @72 */
    read(0, g_buf, sizeof g_buf);      /* 1) stage the real chain in the global */
    read(0, buf, 0x60);                /* 2) SHORT overflow: only 0x60 bytes,
                                        *    room for padding + rbp + one gadget */
    puts("thanks for the input");      /* runs before the ret, like vuln.c      */
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin,  NULL, _IONBF, 0);
    puts("=== ch08 rop pivot lab: chain lives in g_buf; pivot to it ===");
    vuln();
    return 0;
}
