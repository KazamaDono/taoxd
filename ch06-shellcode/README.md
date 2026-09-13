# Chapter 6 — Shellcode for the Modern Age

Companion code for Chapter 6 of *Modern Exploit Development*. Hand-written execve shellcode for x86-64 and AArch64, a small mmap-based loader, and a single-byte XOR encoder that shows how to avoid forbidden bytes. **Lab use only** — see [`../ETHICS.md`](../ETHICS.md).

## Build & run

```bash
./scripts/lab.sh                 # from the repo root, drop into the lab
make -C ch06-shellcode test
```

The test builds each payload, runs the execve shellcode, and confirms the shell it spawns really executes a command:

```text
==> x86-64 execve shellcode
  PASS x86-64 execve
==> AArch64 execve shellcode (under qemu-aarch64-static)
  PASS AArch64 execve
==> XOR encoder round-trip on the execve blob
  PASS xor encoder (key=0x03, ~24 bytes, no NUL/CR/LF)
```

If the AArch64 cross-toolchain (`aarch64-linux-gnu-gcc`) or `qemu-aarch64-static` is unavailable, that stage is skipped without failing the test.

## Contents

| Path | What it is |
|------|------------|
| `src/execve_x64.S` | 24-byte null-free `execve("/bin/sh", NULL, NULL)` payload, x86-64 Linux SysV |
| `src/execve_arm64.S` | AArch64 equivalent using `svc #0` and `x8=221` |
| `src/revshell_x64.S` | Reverse-shell teaching payload; source is kept for reading only, no dial-out from tests |
| `harness/run_shellcode.c` | mmap-based loader (write then `mprotect` to execute) — respects W^X |
| `encoders/xor_encode.py` | Single-byte XOR encoder that finds a key avoiding a forbidden set |
| `Makefile` | Build + `test` target that CI runs |

The revshell payload is intentionally *not* run from tests — this book never dials home from an exercise. It is included as a static teaching artifact.
