/* ch08-rop/target/rop_gadgets.S - guaranteed ROP gadgets for the x86-64 lab.
 *
 * WHY THIS FILE EXISTS (teaching note):
 * glibc >= 2.34 removed __libc_csu_init, which historically donated the
 * canonical `pop rdi ; ret` gadget to every dynamically-linked binary. On
 * Ubuntu 24.04 (glibc 2.39) a trivial -no-pie program may therefore NOT
 * contain `pop rdi ; ret` at a fixed in-binary address, which would break the
 * chapter's Stage-1 leak (it has to set rdi BEFORE any libc leak, so it can
 * only use gadgets already in the executable). We emit the classic arg-load
 * gadgets here explicitly so ROPgadget/ropper/pwntools reproduce the lesson
 * exactly as printed in Listing lst-find, on the pinned toolchain.
 *
 * These are ordinary bytes in an ordinary executable .text page: NX is still
 * fully enforced. Nothing here is executed unless a ROP chain returns into it.
 * x86-64 only; assembled only for the x86-64 build (see Makefile).
 */
#if defined(__x86_64__)
    .section .text
    .globl rop_gadgets
    .type rop_gadgets, @function
rop_gadgets:
    .byte 0x5f, 0xc3            /* pop rdi ; ret */
    .byte 0x5e, 0xc3            /* pop rsi ; ret */
    .byte 0x5a, 0xc3            /* pop rdx ; ret */
    .byte 0x58, 0xc3            /* pop rax ; ret */
    .byte 0x94, 0xc3            /* xchg eax, esp ; ret  (pivot aid) */
    .byte 0xc9, 0xc3            /* leave ; ret          (pivot gadget) */
    .byte 0xc3                  /* ret  (bare, for 16-byte alignment) */
    .size rop_gadgets, .-rop_gadgets

    /* Mark the stack non-executable (NX stays ON) for this object too. */
    .section .note.GNU-stack,"",@progbits
#endif
