/* Toy MMIO device model. Compiled with -fsanitize=address so the OOB is
 * loud when you exercise it. Illustrative of QEMU-style PIO/MMIO handlers.
 * Lab-only; not a hypervisor. See ch28-vbs-hypervisor/README.md.        */
#include <stdint.h>
#include <string.h>
#include "device.h"

#define REG_LEN      0x00       /* w:  bytes to copy on next doorbell    */
#define REG_WINDOW   0x08       /* w:  guest-side source address (mock)  */
#define REG_DOORBELL 0x10       /* w:  any write = "go"                  */

/* Called by the "hypervisor" when the guest writes value at offset off. */
void dev_mmio_write(struct dev_state *d, uint32_t off, uint64_t value)
{
    switch (off) {
    case REG_LEN:
        d->length = (uint32_t)value;                          /* 2 */
        break;
    case REG_WINDOW:
        d->window = value;
        break;
    case REG_DOORBELL: {
        const uint8_t *src = guest_translate(d->window);      /* mocked  */
        /* 3 BUG: no bound check against sizeof(d->rx_slot). */
        memcpy(d->rx_slot, src, d->length);
        if (d->on_complete) d->on_complete(d);                /* 4 */
        break;
    }
    default: /* unknown register: ignore, as real hardware often does */ ;
    }
}
