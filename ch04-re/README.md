# Chapter 4 — companion code

Reverse-engineering lab for Chapter 4, *A Crash Course in Reverse Engineering*.
The target is a self-contained **crackme**: a 16-byte key validator built with
every modern mitigation on. You recover its key statically (Ghidra → solver),
and bypass it dynamically (Frida). **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch04-re test        # build the crackme variants and run the solver
```

`make test` is the chapter's "it works" guarantee: it builds the crackme,
inverts the decompiled transform in `solve/solve.py`, runs each binary with the
recovered key, and asserts it prints `Correct!`. CI runs it on every push on
both the x86-64 and the arm64 runner.

## Contents

| Path | What it is |
|------|------------|
| `src/crackme.c` | The lab target (`lst-crackme`): a 16-byte key check — length gate, then a position-dependent XOR with CBC-style feedback (`c = s[i] ^ (0x5a+i) ^ prev`, `prev` seeded at `0x2a`) compared against an embedded `target[]`. The source is shown in the chapter only so you can check the decompiler; in real work you reverse the binary. |
| `Makefile` | Builds `crackme-x86_64`, `crackme-x86_64.stripped`, and `crackme-arm64` (plus `crackme-arm64.stripped`) with **explicit, commented** mitigation flags (PIE / NX / Full RELRO / stack canary / FORTIFY, all on), and provides `make test`. |
| `solve/solve.py` | pwntools solver (`lst-solve`). Inverts the transform to regenerate the key `R3v3rs1ng_1s_fun` from the extracted `target[]` constants, then runs the binary and asserts the success string. Basis of `make test`. |
| `ghidra/find_checks.py` | Ghidra 11.x Jython script (`lst-ghidra-script`): decompiles every function that calls both a length routine and a comparison routine — the "scripting for scale" / variant-hunting example. Run from the Script Manager or `analyzeHeadless`; **not exercised by CI** (needs Ghidra). |
| `frida/hook.js` | Frida `Interceptor` hook (`lst-frida`): logs `check_serial`'s candidate key and force-replaces its return value to bypass the check. Resolves the symbol via `DebugSymbol.fromName` (symbolized) or `module.base + offset` (stripped); `args[0]` works for both `rdi` and `x0`. **Interactive**, not a CI unit (needs Frida + a live process). |

> The `0x11a9` offset in `frida/hook.js` and the addresses in the chapter's gdb
> session are **illustrative** — read the real offset for your build out of
> Ghidra. The algorithm and constants are exact.

## Which binaries get built where

The Makefile builds whatever the host can target and runs foreign-arch binaries
under `qemu-user`:

| Host | Built & tested natively | Built & tested under QEMU |
|------|-------------------------|----------------------------|
| x86-64 | `crackme-x86_64`, `crackme-x86_64.stripped` | `crackme-arm64` (`qemu-aarch64-static`) |
| arm64  | `crackme-arm64`, `crackme-arm64.stripped` | — (x86-64 skipped unless an x86_64 cross-gcc is installed) |

Stripping removes `.symtab` (so `nm` goes quiet and `check_serial` becomes
`FUN_…`) but **keeps the dynamic imports** (`strlen`, `puts`), exactly as
`tbl-stripped` describes.

## Expected `make test` output

On an x86-64 host (trimmed; pids/paths vary, addresses are randomized by PIE):

```
gcc -std=c11 -O1 -fno-inline -Wall -Wextra -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=3 -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -o crackme-x86_64 src/crackme.c
cp -f crackme-x86_64 crackme-x86_64.stripped
strip --strip-all crackme-x86_64.stripped
aarch64-linux-gnu-gcc ... -o crackme-arm64 src/crackme.c
cp -f crackme-arm64 crackme-arm64.stripped
aarch64-linux-gnu-strip --strip-all crackme-arm64.stripped
built: crackme-x86_64 crackme-x86_64.stripped crackme-arm64 crackme-arm64.stripped
== x86-64 (native) ==
---- solving crackme-x86_64 ----
[+] recovered key: b'R3v3rs1ng_1s_fun'
[+] Starting local process '/work/ch04-re/crackme-x86_64': pid 4711
[+] Receiving all data: Done (65B)
[*] Process '/work/ch04-re/crackme-x86_64' stopped with exit code 0 (pid 4711)
[+] './crackme-x86_64' accepted the key
---- solving crackme-x86_64.stripped ----
[+] recovered key: b'R3v3rs1ng_1s_fun'
[+] './crackme-x86_64.stripped' accepted the key
== aarch64 (under qemu) ==
---- solving crackme-arm64 ----
[+] recovered key: b'R3v3rs1ng_1s_fun'
[+] './crackme-arm64.run' accepted the key
---- solving crackme-arm64.stripped ----
[+] recovered key: b'R3v3rs1ng_1s_fun'
[+] './crackme-arm64.run' accepted the key

ch04-re: all built crackme variants solved — make test OK
```

`make test` exits `0` only if every built variant printed `Correct!`.

## Sanity-check the mitigations

```bash
make -C ch04-re checksec      # or: pwn checksec ch04-re/crackme-x86_64
```

should report `Full RELRO`, `Canary found`, `NX enabled`, `PIE enabled`, matching
the triage in the chapter (`lst-triage`).

## Doing it by hand (the chapter's workflow)

```bash
file crackme-x86_64                      # 64-bit PIE, not stripped
pwn checksec crackme-x86_64              # RELRO/Canary/NX/PIE all on
nm crackme-x86_64 | grep ' t \| T '      # t check_serial / T main
strings -n 6 crackme-x86_64 | grep -i correct

# static: load in Ghidra, decompile check_serial, read target[] + IV, invert.
python3 solve/solve.py ./crackme-x86_64  # -> recovered key, "Correct!"

# dynamic bypass (needs Frida): force the check to pass for any wrong key.
frida -f ./crackme-x86_64 -l frida/hook.js -- WRONG-KEY-GOES-HERE
```
