/* device.h — public interface for the toy MMIO device model and the
 * mock guest_translate helper. See device.c for the deliberately buggy
 * write handler that this chapter's exploit drives.                     */
#ifndef CH28_TOY_DEVICE_H
#define CH28_TOY_DEVICE_H

#include <stdint.h>

struct dev_state {
    uint32_t length;
    uint64_t window;                          /* mock guest pointer      */
    uint8_t  rx_slot[64];                     /* fixed receive buffer    */
    void   (*on_complete)(struct dev_state*); /* 1 called after copy   */
};

/* Called by the "hypervisor" when the guest writes `value` at `off`.    */
void dev_mmio_write(struct dev_state *d, uint32_t off, uint64_t value);

/* Mock guest-physical to host-virtual translator. In a real VMM this
 * would walk the guest's page tables (or shadow tables) and pin the
 * page; here the "guest pointer" is just a host address we hand back.  */
const uint8_t *guest_translate(uint64_t gpa);

#endif /* CH28_TOY_DEVICE_H */
