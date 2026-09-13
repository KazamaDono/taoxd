# The Art of Exploit Development — Companion Code

Working, tested code for every chapter of the book *The Art of Exploit Development: Vulnerability Research and Exploitation on Today's Hardened Systems*.

Every listing in the book maps to a real file here. Every runnable lab ships a test that asserts the exploit does what the chapter claims, and **continuous integration builds the lab and runs those tests on every push** — that is how we keep the code honest.

> ⚠️ **Read this first.** Everything in this repository is for **education, defensive research, and authorized testing only**. The targets are deliberately vulnerable teaching artifacts and version-pinned lab binaries. Run them **only** inside the provided isolated lab (a container, or for kernel work a throwaway VM), on a machine you own. See [`ETHICS.md`](ETHICS.md). If you are not sure something is authorized, don't do it.

## Quick start

You need Docker (Linux, macOS, or Windows via WSL2). Everything else is inside the image.

```bash
git clone git@github.com:<your-username>/art-of-exploit-development.git
cd art-of-exploit-development

# Build the pinned Ubuntu 24.04 lab image (x86-64 + AArch64 cross tools)
./scripts/lab.sh --build

# Drop into the lab shell
./scripts/lab.sh

# Inside the lab: verify your toolchain, then run a chapter's tests
./scripts/doctor.sh
make -C ch05-stack test
```

To run **all** CI-eligible tests the way CI does:

```bash
./scripts/lab.sh make test-all
```

## What's in the lab image

Pinned to **Ubuntu 24.04 LTS**, so offsets and behavior are reproducible:

- glibc **2.39**, gcc **13**, clang/LLVM **18**
- **pwntools**, **pwndbg**, gdb, ROPgadget, ropper, one_gadget, patchelf, seccomp-tools
- **AArch64** cross-toolchain + `qemu-user` and `qemu-system-aarch64` (also used for **MTE/PAC/CET** experiments)
- Fuzzing: AFL++, libFuzzer (clang), honggfuzz; sanitizers (ASan/UBSan/MSan)
- Research tooling: CodeQL CLI, Semgrep, weggli, angr

See [`lab/Dockerfile`](lab/Dockerfile) for the exact pins.

## Layout

```
chNN-slug/        one directory per chapter
  README.md            what's here, how to build, expected output
  Makefile             `make` builds targets, `make test` proves the exploit
  target/ exploit/ ... sources
common/                shared harness helpers (C + Python)
lab/                   Dockerfile + compose for the reproducible environment
scripts/               lab.sh, doctor.sh, build-all.sh, test-all.sh
.github/workflows/     CI: build image, run every `make test` on Ubuntu x86-64 + arm64
```

## How the labs are structured

Each vulnerable target is built with **explicit, commented mitigation flags** so you always know exactly which defense is on or off and why. For example, an early stack-overflow target is compiled `-fno-stack-protector -z execstack -no-pie` **on purpose**, and the Makefile says so; later chapters turn every mitigation back on and defeat it properly.

Tests are deterministic: where an exploit needs a fixed address, the lab disables ASLR *inside the container* (`setarch -R` or the kernel VM's boot args), never on your host.

## Chapter index

| Ch | Topic | Ch | Topic |
|----|-------|----|-------|
| 1 | Mindset & threat model | 18 | Windows Segment Heap |
| 2 | The research lab | 19 | Browser & sandbox anatomy |
| 3 | x86-64 & AArch64 arch | 20 | JS engine internals |
| 4 | Reverse engineering | 21 | JIT: addrof/fakeobj |
| 5 | Stack corruption | 22 | R/W → RCE in the renderer |
| 6 | Modern shellcode | 23 | Sandbox escape |
| 7 | Format string & integer bugs | 24 | Kernel intro |
| 8 | ROP | 25 | Linux kernel I |
| 9 | Advanced code reuse | 26 | Linux kernel II |
| 10 | ASLR & info leaks | 27 | Windows kernel |
| 11 | Canaries, RELRO, FORTIFY | 28 | VBS / hypervisor |
| 12 | CFI / CFG / XFG | 29 | Fuzzing I |
| 13 | CET / PAC / MTE | 30 | Fuzzing II |
| 14 | Heap internals (glibc 2.39) | 31 | Static & variant analysis |
| 15 | Heap primitives | 32 | Symbolic execution |
| 16 | House techniques | 33 | Patch diffing |
| 17 | UAF & type confusion | 34 | The full chain |

Some chapters (Windows kernel/heap, hardware-tag demos) are **not exercised by the Linux CI**; their directories ship build scripts and are marked accordingly. Everything the Linux CI *can* run, it runs.

## License

Code is released under the [MIT License](LICENSE). The book text is not included in this repository.
