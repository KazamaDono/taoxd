# Chapter 24 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 24
of *Modern Exploit Development*. **Lab use only** — see [`ETHICS.md`](ETHICS.md).

Chapter 24 stands up the kernel-exploitation lab you use for the rest of
Part VI: a pinned Linux 6.8 kernel in QEMU, a small vulnerable LKM
(`vuln.ko`) with a kernel-stack overflow in `write(2)` and a KASLR-leak
`ioctl(2)`, a `trigger` client that trips the bug, and a gdb-stub debug
workflow.

## Contents

| Path | What it is |
|------|------------|
| [`vuln/vuln.c`](vuln/vuln.c) | Misc-device LKM with a deliberate kernel-stack overflow in `write()` and a KASLR-leak `ioctl()`. |
| [`vuln/Makefile`](vuln/Makefile) | Kbuild wrapper that builds `vuln.ko` against the running (or specified) 6.8 kernel headers. |
| [`exploit/trigger.c`](exploit/trigger.c) | Userland client: opens `/dev/vuln` and issues a 1024-byte `write(2)`. |
| [`exploit/Makefile`](exploit/Makefile) | Builds `trigger` for x86-64; `make aarch64` cross-builds for AArch64. |
| [`lab/run.sh`](lab/run.sh) | Boots the pinned 6.8 `bzImage` + initramfs in QEMU with SMEP/SMAP forced on. `DEBUG=1` adds `-s -S` for gdb. |
| [`lab/debug.sh`](lab/debug.sh) | Attaches `gdb` (+ pwndbg) to the QEMU stub on `:1234` with `vmlinux` and module symbols. |
| [`lab/BUILD.md`](lab/BUILD.md) | Step-by-step for the one-time pinned Linux 6.8 + BusyBox initramfs build. |
| [`Makefile`](Makefile) | Top-level; `make all` builds the module + trigger, `make test` explains the CI stance. |
| [`ETHICS.md`](ETHICS.md) | Do-not-load notice; the module is world-writable on purpose. |
| [`.ci-skip`](.ci-skip) | Marker that keeps this chapter out of the shared Ubuntu CI runner. |

## Prerequisites

For **build** (all steps except the end-to-end boot):

- Ubuntu 24.04, gcc 13, clang 18 (repo-pinned defaults from
  [`../../bible/tech-stack.md`](../../bible/tech-stack.md)).
- Kernel headers matching the running kernel:
  `sudo apt-get install linux-headers-$(uname -r) build-essential`.
- Optional: `gcc-aarch64-linux-gnu` for exercise 6.

For **run** (the QEMU lab):

- `qemu-system-x86_64` (also `qemu-system-aarch64` for the ARM path).
- A pinned `bzImage` + `vmlinux` + `initramfs.cpio.gz` under
  `lab/artifacts/`. Build them once with the recipe in
  [`lab/BUILD.md`](lab/BUILD.md); the artifacts are large and not
  committed.

## Build

```bash
# from this directory
make all          # -> vuln/vuln.ko and exploit/trigger
```

## Run the lab

```bash
./lab/run.sh                 # boot; on trigger, expect a kernel panic
# in a second terminal, from a shell inside the VM:
/bin/trigger                 # tries write(fd, "A"*1024, 1024)
```

Expected sequence: the VM prints `vuln: took 1024 bytes, first=0x41`,
followed by a kernel-side stack-smashing detection and a call trace ending
in `__stack_chk_fail` — because `panic=-1 oops=panic` is set in
`run.sh`, the VM stops there and QEMU exits (thanks to `-no-reboot`).

## Debug

```bash
DEBUG=1 ./lab/run.sh         # boots frozen with -s -S
# second terminal:
export MOD_BASE=$(...)       # read from the serial console printout
./lab/debug.sh               # attaches gdb -> break vuln_write -> continue
```

## `make test` expected output

CI honors [`.ci-skip`](.ci-skip) for this chapter, so `make test` here is a
non-destructive placeholder:

```
=== ch24: CI-skipped ===
Requires a pinned Linux 6.8 bzImage + vmlinux + BusyBox initramfs built per lab/BUILD.md (10-20 min on a laptop) and a qemu-system-x86_64 boot whose only success signal is a kernel panic on __stack_chk_fail — too heavyweight for the shared Ubuntu CI runner. Build/run scripts and the vulnerable module ship here; see README.md and lab/BUILD.md to reproduce locally.
CH24_CI_SKIP_OK
```

On a lab host with the pinned toolchain and matching kernel headers,
`make test-build` additionally builds `vuln.ko` and `trigger` and prints
`CH24_BUILD_OK`. The full end-to-end run of the exploit belongs to the
QEMU workflow above.

## Where this goes next

Chapter 25 keeps the same LKM but treats the canary and KASLR as the first
obstacles instead of endpoints; Chapter 26 promotes the bug from an oops
into a controlled primitive. All of Part VI reuses `vuln/vuln.c` and the
QEMU lab this chapter builds.
