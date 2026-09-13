# KVM / QEMU reading guide — where the real device-escape bugs live

This is the follow-on reading for Chapter 28. The toy device in
`toy-device/device.c` shows the *shape* of a device-emulation VM escape;
the files below are where the shape shows up in production code. Read
them with the three-slot template from the chapter in mind: **which
channel is the entry point, which host allocation gets corrupted, what
does the code do with that corruption in the same transaction?**

## Where the code lives

- **KVM core.** Linux tree, `arch/x86/kvm/` (vmx, svm, mmu, x86.c) plus
  `virt/kvm/`. Small, tightly reviewed. Read `handle_ept_violation()`
  and follow the dispatch into userland. This is the part that almost
  never has escape-grade bugs.
- **QEMU device models.** `hw/` in the QEMU tree. Millions of lines.
  Start with:
  - `hw/net/e1000.c` and `hw/net/e1000e_core.c` — the classic emulated
    NIC; a long history of length-field and descriptor-parsing bugs.
    Compare `e1000_receive_iov()` to `dev_mmio_write()` in this
    chapter's lab.
  - `hw/usb/hcd-xhci.c` — richer state machine, per-transaction
    descriptor chains; CVE-2020-14364 lived nearby.
  - `hw/block/fdc.c` — the QEMU floppy controller. `fdctrl_write_data()`
    is the file that VENOM (CVE-2015-3456) exploited. The two-line fix
    is essential reading against the lab's device.c.
  - `hw/scsi/scsi-disk.c`, `hw/scsi/megasas.c` — SCSI/megasas emulation
    has produced a steady stream of CVEs; complex state machines with
    guest-controlled sense buffers.
  - `hw/display/virtio-gpu.c`, `hw/display/qxl.c` — display devices,
    perennially leaky because of large per-command payloads.
- **virtio backends.** `hw/virtio/` and `hw/net/virtio-net.c`. The
  para-virt equivalent of the emulated devices above; leaner but the
  bug shapes carry over.
- **Documentation.** Linux tree, `Documentation/virt/kvm/` — the API
  contract you are attacking. Read `api.rst` and `mmu.rst` first.

## Notable VM-escape CVEs to study

Use these as case studies. For each, answer the three-slot template
before reading the fix.

| CVE | Component | Bug shape |
|---|---|---|
| CVE-2015-3456 (VENOM) | QEMU `hw/block/fdc.c` | Guest-controlled length into fixed FIFO; adjacent callback fires |
| CVE-2015-5165 | QEMU RTL8139 | Uninitialised read leaks host memory to guest |
| CVE-2016-3710 (Dark Portal) | QEMU VGA | Bounds-check bypass in banked VGA writes |
| CVE-2019-6778 | QEMU SLIRP | Heap overflow in `tcp_emu()` |
| CVE-2019-14378 | QEMU SLIRP | Heap overflow in IP reassembly |
| CVE-2020-14364 | QEMU USB | OOB R/W in USB packet handling |
| CVE-2021-3947 | QEMU NVMe | Buffer overflow in NVMe controller |
| CVE-2023-3255 | QEMU vhost-user-fs | UAF in fuse request handling |

The Google Project Zero blog and the QEMU security advisories page
(`https://www.qemu.org/docs/master/system/security.html`) are the two
best public trackers.

## Setting up a local KVM/QEMU debugging lab

The lab in this repo does not need KVM; the toy device runs as a
plain userland process so CI can execute it. For a *real* device-model
debugging session, build a stock QEMU with symbols and drive it with
gdb:

```bash
# On Ubuntu 24.04, from a source checkout of qemu
./configure --enable-debug --enable-sanitizers \
            --target-list=x86_64-softmmu
make -j$(nproc)

# Launch with a gdbstub on :1234 and expose the built-in monitor
./build/qemu-system-x86_64 \
    -enable-kvm -m 2G -nographic \
    -kernel /boot/vmlinuz -append "console=ttyS0" \
    -netdev user,id=n0 -device e1000,netdev=n0 \
    -s -S

# In another shell:
gdb --args ./build/qemu-system-x86_64 ...
(gdb) target remote :1234
(gdb) b e1000_receive_iov
(gdb) c
```

From here you drive the guest to hit the emulated device and watch the
handler execute in the host process. Reproducing a public CVE this way
is the fastest route to internalising the shapes in the table above.

## Further reading

- Google Project Zero, *Adventures in Xen Exploitation* (2015).
- J. Geffner, *VENOM: Virtualized Environment Neglected Operations Manipulation* (2015).
- QEMU security advisories: <https://www.qemu.org/docs/master/system/security.html>
- Linux `Documentation/virt/kvm/` in-tree docs.
