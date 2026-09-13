/* ch08-rop/target/rop_gadgets_arm64.S - guaranteed AArch64 epilogue gadgets.
 *
 * WHY THIS FILE EXISTS (teaching note):
 * On AArch64 there are NO unintended, mid-instruction gadgets (instructions
 * are a fixed 4 bytes, 4-byte aligned), so a tiny -no-pie binary simply does
 * not contain the epilogue-shaped gadgets a ret2libc chain needs to set the
 * argument register x0 and to chain calls. Real non-leaf functions donate
 * these gadgets; a trivial lab target does not. We provide the canonical
 * function-epilogue gadgets here, at fixed (no-PIE) addresses, so the chapter's
 * "chain function-epilogue gadgets that reload x30" lesson reproduces under
 * qemu-aarch64 on the pinned toolchain. NX is still fully enforced — these are
 * ordinary .text bytes, only ever reached by a chain that returns into them.
 * AArch64 only (guarded); the x86-64 build never sees this file.
 */
#if defined(__aarch64__)
    .section .text

    /* arm_set_x0: load x0 from the stack and a fresh x30 (next gadget), then
     * return. The universal AArch64 chaining shape: every gadget must reload
     * x30 from the stack, or every ret would go to the same place. */
    .globl arm_set_x0
    .type  arm_set_x0, %function
arm_set_x0:
    ldr  x0,  [sp]             // x0  <- arg
    ldr  x30, [sp, #8]         // x30 <- next gadget / return target
    add  sp,  sp, #16
    ret
    .size arm_set_x0, .-arm_set_x0

    /* arm_call: load arg -> x0, target -> x8, continuation -> x30, then
     * tail-branch (br) into x8. Because br does NOT write x30, the callee
     * returns (via its own ret) to the continuation we staged. This is a
     * reliable "call f(arg) and keep chaining" primitive for AArch64 ROP. */
    .globl arm_call
    .type  arm_call, %function
arm_call:
    ldp  x0,  x8,  [sp, #0]    // x0 <- arg,  x8 <- function to call
    ldr  x30,      [sp, #16]   // x30 <- where the callee returns (next link)
    add  sp,  sp,  #24
    br   x8
    .size arm_call, .-arm_call

    /* arm_ldp_x0: the exact shape used in Listing lst-arm, provided so the
     * book's "ldp x0, x1, [sp, #0x10] ; ldp x29, x30, [sp], #0x20 ; ret"
     * gadget has a fixed home even when libc's copy is not a clean match. */
    .globl arm_ldp_x0
    .type  arm_ldp_x0, %function
arm_ldp_x0:
    ldp  x0,  x1,  [sp, #0x10]
    ldp  x29, x30, [sp], #0x20
    ret
    .size arm_ldp_x0, .-arm_ldp_x0

    .section .note.GNU-stack,"",%progbits
#endif
