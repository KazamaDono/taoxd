/* ch09-codereuse/srop/vuln_arm64.c — AArch64 port of the SROP target.
 * Lab-only. See ../../ETHICS.md. Run under qemu-aarch64.
 *
 * AArch64 SROP differs from x86-64 in three places (Exercise 5):
 *   - the rt_sigreturn syscall number is 139 (x86-64: 15);
 *   - the syscall number goes in x8, not rax;
 *   - the frame fields are x0/pc/sp (not rdi/rip/rsp).
 *
 * Built deliberately weak: no canary, no PIE, so the gadget/string addresses
 * are static. A linear overflow of buf overwrites the *caller's* saved x30
 * (AArch64 saves fp/lr at the lowest frame address, below the buffer, so the
 * overflow reaches the return address one frame up).
 *
 * Planted gadgets, exported for the solver:
 *   sigreturn_trigger : mov x8, #139; svc #0   -> invoke rt_sigreturn
 *   svc_gadget        : svc #0                  -> generic syscall; the forged
 *                       frame sets x8=221 (execve) and pc=svc_gadget. */
#include <unistd.h>

__asm__(
    ".globl sigreturn_trigger\n"
    "sigreturn_trigger: mov x8, #139; svc #0\n"
    ".globl svc_gadget\n"
    "svc_gadget: svc #0\n"
    ".section .rodata\n"
    ".globl binsh\n"
    "binsh: .asciz \"/bin/sh\"\n"
    ".text\n");

void vuln(void) {
    char buf[32];
    read(0, buf, 512);
}

int main(void) {
    vuln();
    return 0;
}
