/* ch09-codereuse/dlresolve/vuln.c — lab-only ret2dlresolve target.
 * See ../../ETHICS.md.
 *
 * Deliberately weak, with the exact conditions ret2dlresolve needs:
 *   - -no-pie            : fixed load address (no leak needed for our own code)
 *   - partial RELRO + lazy: the .got.plt stays writable and PLT[0] is a live
 *                           lazy resolver trampoline -> dlresolve is reachable.
 *                           (Full RELRO / -z now would kill this technique.)
 *   - no stack canary    : the linear overflow below can reach the return addr.
 *
 * On glibc >= 2.34 there is no __libc_csu_init, so the small arg-setting
 * gadgets that chain used to borrow are gone. We therefore PLANT a tiny,
 * self-contained gadget block (pop rdi/rsi/rdx; ret) so the first-stage ROP
 * chain can set up read()/the resolver call without a leak. This mirrors the
 * real-world situation the chapter describes: you still need a handful of
 * stack-pivot / arg gadgets to drive the resolver, you just don't need libc. */
#include <unistd.h>

__asm__(
    ".text\n"
    ".globl gadget_pop_rdi\n"
    "gadget_pop_rdi: pop %rdi; ret\n"
    ".globl gadget_pop_rsi\n"
    "gadget_pop_rsi: pop %rsi; ret\n"
    ".globl gadget_pop_rdx\n"
    "gadget_pop_rdx: pop %rdx; ret\n");

void vuln(void) {
    char buf[64];
    read(0, buf, 0x400);   /* linear overflow: 0x400 bytes into a 64-byte buf */
}

int main(void) {
    vuln();
    return 0;
}
