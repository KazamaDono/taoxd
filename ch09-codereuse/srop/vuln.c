/* ch09-codereuse/srop/vuln.c — lab-only SROP target. See ../../ETHICS.md.
 * Built deliberately weak: no canary, no PIE, ASLR off in the lab, so the
 * gadget and string addresses are static. The point is SROP, not ASLR. */
#include <unistd.h>

/* Provided gadgets + string, exported so the solver can read their
 * addresses straight from the symbol table. */
__asm__(
    ".globl syscall_ret\n"
    "syscall_ret: syscall; ret\n"          /* ❶ */
    ".globl pop_rax_ret\n"
    "pop_rax_ret: pop %rax; ret\n"         /* ❷ */
    ".section .rodata\n"
    ".globl binsh\n"
    "binsh: .asciz \"/bin/sh\"\n"          /* ❸ */
    ".text\n");

void vuln(void) {
    char buf[32];
    read(0, buf, 512);                     /* ❹ 512 bytes into a 32-byte buffer */
}

int main(void) {
    vuln();
    return 0;
}
