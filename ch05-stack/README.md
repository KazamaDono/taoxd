# Chapter 5 — companion code

Stack-buffer-overflow lab for Chapter 5, *Stack-Based Memory Corruption
Revisited*. One deliberately vulnerable source (`src/vuln.c`) is built **two
ways** so you can see, side by side, why the 1996-era return-address overwrite
fails on a modern default build and works the instant you turn the stack
defenses off. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch05-stack test     # build all variants and run the ret2win solver
make -C ch05-stack demo-hardened   # watch the hardened build abort on the canary
```

`make test` is the chapter's "it works" guarantee: it builds the unprotected
target and runs `solve/solve.py`, which overwrites `greet`'s saved return
address with `win()` and asserts a **real shell** (it runs `echo PWNED_$((13*3))`
and checks for `PWNED_39`). It exits **0 only on a proven shell**: the solver
exits non-zero on failure *and* the recipe greps its output for the unique
`ret2win OK` marker.

CI runs `make test` on every push on **both** an x86-64 runner and an arm64
runner, so each architecture is exercised natively. On each runner the
**native** architecture is the hard gate; the **foreign** architecture is built
and solved best-effort under `qemu-user` (non-fatal, since the other runner
gates it natively). The recipe sets `kernel.core_pattern` (best-effort,
passwordless `sudo -n` in CI) so the solver's core dump is found deterministically,
and runs the solver under `setarch -R` (ASLR off) for reproducibility.

## Contents

| Path | What it is |
|------|------------|
| `src/vuln.c` | The lab target (`lst-vuln`): a 64-byte stack buffer and `read(fd, buf, 512)` via a `noinline` helper so FORTIFY cannot see the object size and the overflow actually happens. Non-static `win()` is the ret2win oracle (pops a shell). |
| `Makefile` | Builds `vuln-<arch>` (canary/PIE/FORTIFY **off**, NX **on** — the exploitable build) and `vuln-<arch>-hardened` (every modern mitigation **on**), with explicit, commented flags (`lst-mits`). Provides `make test`, `make demo-hardened`, `make checksec`. |
| `solve/solve.py` | pwntools ret2win solver (`lst-ret2win`). Finds the saved-return-address offset at runtime with a cyclic pattern + core dump (no hardcoded constant), prepends one `ret` gadget to fix x86-64 `movaps` stack alignment, jumps to `win()`, and asserts a real shell (`PWNED_39`). Arch-aware, so it runs unchanged on the AArch64 build (saved `x30`, offset 80). Basis of `make test`. |

## Which binaries get built where

| Host | Built & tested natively | Built & tested under QEMU |
|------|-------------------------|----------------------------|
| x86-64 | `vuln-x86_64`(+`-hardened`) | `vuln-arm64` (`qemu-aarch64-static`) |
| arm64  | `vuln-arm64`(+`-hardened`) | `vuln-x86_64` (if an x86_64 cross-gcc is installed) |

## Mitigation flags (why each is set)

The exploitable build turns off exactly the defenses this chapter motivates, and
**no more** — NX stays on, because ret2win reuses existing code:

```
vuln       : -O0 -fno-stack-protector -no-pie -fno-pie -D_FORTIFY_SOURCE=0
             -Wl,-z,noexecstack                       # canary/PIE/FORTIFY off, NX on
vuln-*-hardened : -O2 -fstack-protector-all -fPIE -D_FORTIFY_SOURCE=3
             -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack   # the modern default
```

```bash
make -C ch05-stack checksec     # confirm the two builds' mitigation tables differ
```

> The `0x40...` addresses and the offset `72` in the chapter's gdb transcript
> are from the `-O0 -no-pie` build and are stable for it; the solver discovers
> the offset at runtime rather than trusting a constant, so it survives compiler
> changes and works unchanged on AArch64 (saved `x30`, not a pushed return).

## Expected `make test` output (trimmed)

On an x86-64 host with the aarch64 cross toolchain + `qemu-aarch64-static`
present (arm64 solved best-effort under emulation):

```
built: vuln-arm64 vuln-arm64-hardened vuln-x86_64 vuln-x86_64-hardened
== x86_64 ret2win ==
[+] ./vuln-x86_64: ret2win OK at offset 72
== arm64 ret2win (best-effort under QEMU) ==
[+] ./vuln-arm64: ret2win OK at offset 80
ok: arm64 ret2win confirmed under emulation

ch05-stack: ret2win confirmed on every built target — make test OK
```

If no cross toolchain/emulator is installed, the foreign line reads
`-- no arm64 binary built; skipping foreign solve` (or a non-fatal
`skipped/failed under emulation` note) and the native `x86_64` solve still
gates the run. On the arm64 CI runner the roles swap: `arm64` is the native
gate and `x86_64` is the best-effort foreign target.
