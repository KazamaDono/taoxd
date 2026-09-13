# Chapter 8 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 8,
*Bypassing DEP/NX with Return-Oriented Programming*, of *Modern Exploit
Development*. **Lab use only** — see `../ETHICS.md`.

Every target is built with explicit, commented mitigation flags (see
`target/Makefile`): **NX/DEP on**, stack canary off, PIE off, CET off. So
injected shellcode is dead and ROP is the only way forward — which is the
whole point of the chapter.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch08-rop test        # build the target(s) and run the exploit tests
```

Every runnable lab ships a `make test` that asserts the exploit reaches its
goal; CI runs it on every push (x86-64 and arm64 runners).

## Contents

| Path | What it is |
|------|------------|
| `target/vuln.c` | The DEP-protected lab target (Listing `lst-vuln`): unbounded `read` into a 64-byte stack buffer. Built NX-on / canary-off / PIE-off / CET-off. |
| `target/vuln_pivot.c` | Dedicated target for the stack-pivot demo: a *short* overflow plus a fixed-address global `g_buf` to stage the real chain in. |
| `target/rop_gadgets.S` | x86-64 epilogue/arg-load gadgets (`pop rdi ; ret`, `leave ; ret`, bare `ret`, …) pinned into the binary so the chapter's workflow reproduces on glibc ≥ 2.34 (which dropped `__libc_csu_init`). |
| `target/rop_gadgets_arm64.S` | AArch64 function-epilogue gadgets (`arm_call`, `arm_set_x0`, `arm_ldp_x0`) — there are no unintended mid-instruction gadgets on AArch64, so a tiny target has none without this. |
| `target/Makefile` | Build rules; every mitigation flag is explicit and commented with *which* defense is on/off and *why*. |
| `exploit/solve_x86_64.py` | Complete two-stage `ret2libc` (Listing `lst-solve`): leak libc via `puts(puts@got)`, return into `vuln()`, then `system("/bin/sh")` with a `movaps`-alignment `ret`. pwntools `ROP()`. |
| `exploit/pivot_demo.py` | Stack-pivot demo (Listing `lst-pivot`): stages the full chain in `g_buf` and uses `leave ; ret` to relocate `rsp` into it. |
| `exploit/solve_arm64.py` | AArch64 `ret2libc` (Listing `lst-arm`): overwrite the spilled `x30` and chain epilogue gadgets to call `system("/bin/sh")`. Runs natively on aarch64 and under `qemu-aarch64` on x86-64. |
| `Makefile` / `ci-test.sh` | `make test` orchestration (hard gates + best-effort lane — see below). |

## What `make test` proves

- **x86-64 `ret2libc`** (`solve_x86_64.py`) and **x86-64 stack pivot**
  (`pivot_demo.py`) are **hard gates**: they must pop a shell in the lab and
  echo the unique `PWNED-<uid>` marker, or the test fails. Both runs are pinned
  with `setarch -R` so stack alignment (the `movaps` footgun) is deterministic.
  libc ASLR is left on; the exploits *leak* libc, they do not hardcode it.
- **AArch64 `ret2libc`** (`solve_arm64.py`) is a **hard smoke gate** on a native
  aarch64 host (the target must build and run) and a **best-effort** full-chain
  run there and under qemu. It is best-effort because the saved-`x30` offset and
  the libc gadget shape are codegen-/build-specific and this lane was authored
  without a local aarch64 box to verify against; the script self-calibrates the
  offset from a corefile where one is available. Verify the full AArch64 shell
  pop on real aarch64 hardware before relying on it.

### Expected output (x86-64 host, abridged)

```
==== build x86-64 targets ====
built vuln-x86_64 (NX on, canary off, PIE off, CET off)
built vuln_pivot-x86_64 (NX on, canary off, PIE off, CET off)
==== x86-64 ret2libc (solve_x86_64.py) ====
[*] '.../exploit/vuln-x86_64'
[+] Starting local process ...
[+] libc base = 0x7f...000
[+] shell confirmed: 0
PASS  x86-64 ret2libc
==== x86-64 stack pivot (pivot_demo.py) ====
[+] libc base = 0x7f...000
[+] shell confirmed: 0
PASS  x86-64 stack pivot
==== build AArch64 target ====
built vuln-arm64 (NX on, canary off, PIE off; AArch64 epilogue gadgets linked)
==== AArch64 smoke (target runs?) ====
PASS  AArch64 target runs
==== AArch64 ret2libc (solve_arm64.py, best-effort) ====
[+] libc base = 0x...
[+] shell confirmed: 0
PASS  AArch64 ret2libc

ch08-rop: all hard gates passed.
```

On the `ubuntu-24.04-arm` CI runner the x86-64 lanes are skipped (no x86-64
toolchain) and the AArch64 lanes run natively.

### Figures referenced by the chapter

The chapter references two hand-authored SVGs — `assets/images/ch08-rop-stack.svg`
and `assets/images/ch08-pivot.svg`. Those live under the manuscript's
`assets/images/` tree, not in this code directory.
