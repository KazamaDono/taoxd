/* lab_main.c — assembles the toy VM-escape lab.
 *
 * Builds into libtoylab.so, exporting:
 *   guest_write(off, val)  — drives the emulated MMIO region
 *   win()                  — the host-side function the exploit targets
 *   escaped()              — CI predicate; nonzero if win() ran
 *
 * Also provides a tiny main() so the shared object can be run standalone
 * (`./libtoylab.so` under a suitable loader, or via the pytest harness).
 * The interesting behaviour is all in device.c; this file is glue.     */
#include <stdint.h>
#include <stdio.h>

extern void guest_write(uint32_t off, uint64_t value);
extern void guest_reset(void);

static volatile int g_escaped = 0;

/* Target of the OOB overwrite. In a real VM escape this would be
 * whatever the attacker chained to on the host — `system("/bin/sh")`
 * in the classic case, or a ROP stager against the VMM process.      */
void win(void *unused)
{
    (void)unused;
    g_escaped = 1;
    /* Unique marker the Makefile greps for. Do not change without also
     * updating the `test` recipe.                                     */
    fputs("CH28-ESCAPED\n", stdout);
    fflush(stdout);
}

int escaped(void)
{
    return g_escaped;
}

/* Standalone entry point: mostly for manual smoke-testing. The Python
 * solver drives the library directly via ctypes and does not call this. */
int main(void)
{
    guest_reset();
    fputs("toylab: no exploit driven; run exploit/solve.py\n", stdout);
    return 0;
}
