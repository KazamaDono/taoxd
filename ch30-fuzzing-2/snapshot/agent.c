/* ch30-fuzzing-2/snapshot/agent.c
 *
 * Minimal guest-side agent for a Nyx-style snapshot fuzzer, targeting the
 * ch24 lab VM. Schematic only — the point is to show the observer channel
 * and the entropy stubs from the chapter, not to be a drop-in replacement
 * for Nyx's `hget`/`hset`.
 *
 * Protocol (all over I/O port 0x1337 on x86-64):
 *   outb  cmd=0x01 => "I am at the fuzz point; take snapshot here."
 *                     The host takes savevm, allocates a shared input page,
 *                     writes the input page's guest physical address into
 *                     EAX via the port, and returns.
 *   outb  cmd=0x02 => "I consumed the input; wake me when the next one
 *                     is placed." The host records coverage and rolls back.
 *   outb  cmd=0x03 => "Crash / assertion tripped." The host records the
 *                     minimized reproducer and rolls back.
 *
 * Build (userland smoke test — no VM needed):
 *   cc -g -O1 -Wall -Wextra -o agent agent.c
 *
 * On a real snapshot run this would be linked into an initramfs and
 * invoked as PID 1 inside the ch24 VM.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 * Entropy stubs — the chapter's warning made concrete.
 *
 * On a real snapshot campaign these are enforced by the host: RDRAND is
 * intercepted via CPUID masking, the TSC is frozen with KVM_SET_TSC at the
 * snapshot instant, and the allocator's newly-mapped pages are zeroed by
 * modifying the kernel's page-allocation path. Here we document them so
 * the reader can see where they belong.
 * ------------------------------------------------------------------------ */
static uint64_t stub_rdrand(void)   { return 0; }
static uint64_t stub_tsc(void)      { return 0; }

/* --------------------------------------------------------------------------
 * Observer channel: the outb-to-0x1337 protocol.
 * On non-x86 hosts we degrade to a printf so the file still builds; the
 * demo target is x86-64. `hypercall` returns whatever the host wrote back
 * (an input-page gpa on cmd=0x01, otherwise 0).
 * ------------------------------------------------------------------------ */
static uint32_t hypercall(uint8_t cmd) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t eax = cmd;
    /* `out` returns; the fuzzer will have populated %eax by the time
     * execution resumes. Uncomment when running as the guest agent —
     * userland cannot issue outb without CAP_SYS_RAWIO / iopl(3). */
    /* __asm__ volatile ("outb %%al, %1" : "+a"(eax) : "Nd"((uint16_t)0x1337)); */
    (void)eax;
    fprintf(stderr, "[agent] hypercall cmd=0x%02x (stubbed)\n", cmd);
    return 0;
#else
    fprintf(stderr, "[agent] hypercall cmd=0x%02x (non-x86 stub)\n", cmd);
    return 0;
#endif
}

int main(void) {
    /* One-shot handshake so the smoke test proves the file links and the
     * observer channel is wired. Real deployment loops on the input page
     * and calls the target under test between hypercalls #1 and #2. */
    uint32_t input_gpa = hypercall(0x01);
    fprintf(stderr, "[agent] input page @ gpa=0x%08x, tsc=%lu, rand=%lu\n",
            input_gpa, (unsigned long)stub_tsc(),
            (unsigned long)stub_rdrand());

    /* Placeholder for: read N bytes from the shared page, drive the ch24
     * chardev with them, catch faults via a signal handler. */

    (void)hypercall(0x02);
    puts("MED_CH30_SNAPSHOT_AGENT_OK");
    return 0;
}
