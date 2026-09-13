# Chapter 28 — companion code

Deliberately vulnerable teaching artifacts and working exploits for
Chapter 28 (*Defeating Virtualization-Based Security*) of
*Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                                 # drop into the Ubuntu 24.04 lab container
make -C ch28-vbs-hypervisor test                 # build the toy device and run the exploit
```

`make test` builds `toy-device/libtoylab.so` under AddressSanitizer,
runs `exploit/solve.py` via pytest, and passes only if:

1. the OOB memcpy in `dev_mmio_write()` overwrites `on_complete`,
2. the toy device then invokes the freshly-planted callback,
3. `win()` prints the unique marker `CH28-ESCAPED`, and
4. AddressSanitizer flags the out-of-bounds write.

## Contents

| Path | What it is |
|------|------------|
| `Makefile` | Top-level build; compiles the toy-device lab under ASan and runs the pytest solver. |
| `hvci-probe/hvci-check.ps1` | PowerShell probe reporting whether VBS/HVCI/Credential Guard are running on a Windows 11 24H2 target (Listing 28-1). Windows-only; not exercised in Linux CI. |
| `toy-device/device.h` | Public interface for the toy MMIO device model and the mock `guest_translate` helper. |
| `toy-device/device.c` | Toy MMIO device model with the deliberate length-field OOB write adjacent to a callback pointer (Listing 28-2). |
| `toy-device/guest.c` | Mock guest driver: `guest_write(off, val)` invokes `dev_mmio_write` the way a real ring-0 driver would trap on PCI MMIO. |
| `toy-device/lab_main.c` | Builds `libtoylab.so`; exports `win()` (the host-side escape target) and the `escaped()` predicate CI asserts on. |
| `exploit/solve.py` | Python solver that drives the toy device into calling `win()` via OOB overwrite of `on_complete` (Listing 28-3). |
| `exploit/test_solve.py` | pytest wrapper invoked by `make test`; verifies `solve.py` exits 0 with the marker and ASan flags the OOB. |
| `kvm-reading-guide.md` | Curated reading guide for real KVM/QEMU device-model source and notable VM-escape CVEs. |

## Expected `make test` output

```
[+] driving toy device via exploit/solve.py under ASan
.                                                                        [100%]
1 passed in 0.4s
.                                                                        [100%]
1 passed in 0.4s
PASS: ch28 toy VM-escape reproduced (CH28-ESCAPED marker seen)
```

The exact ASan wording varies by libasan version, but the pytest
assertion accepts any of `global-buffer-overflow`,
`heap-buffer-overflow`, or `stack-buffer-overflow` on the OOB memcpy
inside `dev_mmio_write`.

## Ethics

The toy device is a plain userland process; it is not a hypervisor and
cannot be pointed at one. The Windows PowerShell probe is read-only and
uses only the documented `Win32_DeviceGuard` WMI class. Nothing in this
directory targets production hypervisors, cloud tenants, or third-party
systems. See `../ETHICS.md` and Appendix D of the book.
