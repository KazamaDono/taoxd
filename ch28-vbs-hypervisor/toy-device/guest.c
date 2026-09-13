/* guest.c — mock guest driver.
 *
 * A real guest kernel driver would write to PCI MMIO addresses, causing
 * an EPT violation that the hypervisor catches and dispatches to a
 * device model. We collapse that path: guest_write() calls the device
 * model's dev_mmio_write() directly. The trap-and-decode scaffolding is
 * uninteresting and is not the buggy part. See ch28.md fig-guest-io.   */
#include <stdint.h>
#include "device.h"

/* Single device instance owned by the "hypervisor" process. It is the
 * host-side allocation that the OOB write in dev_mmio_write() smashes. */
static struct dev_state g_dev;

/* The mock guest_translate() just casts the "guest pointer" back to a
 * host pointer. The solver stuffs a real host address in REG_WINDOW.  */
const uint8_t *guest_translate(uint64_t gpa)
{
    return (const uint8_t *)(uintptr_t)gpa;
}

/* Exposed to the Python solver via ctypes. Mirrors the shape of a
 * ring-0 guest driver writing to a device BAR.                        */
void guest_write(uint32_t off, uint64_t value)
{
    dev_mmio_write(&g_dev, off, value);
}

/* Reset device state between runs (useful when driving from tests).  */
void guest_reset(void)
{
    g_dev.length = 0;
    g_dev.window = 0;
    g_dev.on_complete = 0;
    for (unsigned i = 0; i < sizeof(g_dev.rx_slot); i++)
        g_dev.rx_slot[i] = 0;
}
