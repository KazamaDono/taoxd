# Chapter 10 — companion code

Deliberately vulnerable teaching artifacts and working exploits for
Chapter 10 (*ASLR and the Art of the Information Leak*) of
*Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch10-aslr test      # builds the leak-server target and runs the exploit
```

Every runnable lab ships a `make test` that asserts the exploit reaches
its goal; CI runs it on every push.

## Contents

| Path | What it is |
|------|------------|
| `tools/compute_bases.py` | Base-computation helpers: turn a leaked pointer + symbol into a region base and any other symbol. Mirrors listing `lst-basemath` from the chapter. |
| `leak-server/vuln.c` | x86-64 lab target: publishes `dlsym(RTLD_DEFAULT, "puts")` as a debug leak, then reads 512 bytes into a 64-byte stack buffer. Canary OFF, PIE OFF, NX ON, ASLR ON. |
| `leak-server/vuln-arm64.c` | AArch64 variant with PIE ON (requires two leaks: libc + PIE base). Cross-built with `aarch64-linux-gnu-gcc` when available. Solver is Exercise 6. |
| `leak-server/Makefile` | Mitigation-annotated build for the x86-64 target and the AArch64 cross build. Provides `all`, `arm64`, `arm64-maybe`, `test`, `clean`. |
| `leak-server/solve.py` | End-to-end solver: parses the banner leak, computes libc base, drops a ret2libc chain (`ret; pop rdi; "/bin/sh"; system`) into the overflow, and hands over an interactive shell. |
| `leak-server/tests/test_solve.py` | pytest harness: builds the solver's chain 5× against fresh processes (fresh libc bases) and asserts a shell responds. Deterministic. Invoked by `make test`. |
| `leak-server/notes/offset.md` | How the buffer→saved-RIP offset of 72 was derived with `cyclic()`/`cyclic_find()`. |
| `partial/vuln.c` | One-byte partial-overwrite lab: callback pointer adjacent to a fixed-size buffer; `win` and `default_cb` in the same 256-byte .text window. |
| `partial/solve.py` | Deterministic one-byte partial-overwrite solve. Reads symbols with `pwn.ELF`, writes `win & 0xff` as the 65th byte, checks for the marker. |
| `partial/Makefile` | Annotated build for the partial lab; post-link `layout` target asserts `win` and `default_cb` share a 256-byte .text window. |
| `Makefile` | Chapter-level Makefile; `make test` (CI gate) delegates to `leak-server/`; `make all-test` also runs `partial/`. |

## Expected `make test` output

Successful CI run of the chapter's gate:

```text
$ make -C ch10-aslr test
make[1]: Entering directory '.../ch10-aslr/leak-server'
gcc -O1 -Wall -Wno-format-security -fno-inline -fno-stack-protector \
    -no-pie -D_GNU_SOURCE -Wl,-z,relro,-z,now -Wl,-z,noexecstack -o vuln vuln.c
CH10_RUNS=5 python3 -m pytest -q tests/test_solve.py
.....                                                            [100%]
5 passed in 3.42s
make[1]: Leaving directory '.../ch10-aslr/leak-server'
```

Five independent runs against fresh execves (each with a freshly
randomised libc base) all pop a shell and echo the unique success
marker `CH10_LEAK_PWNED_57ac1e`.

## Running the partial-overwrite lab locally

```text
$ make -C ch10-aslr/partial test
gcc -O0 -Wall -Wno-unused-result -fno-inline -fno-stack-protector \
    -no-pie -D_GNU_SOURCE -Wl,-z,relro,-z,now -Wl,-z,noexecstack -o vuln vuln.c
layout OK: win=0x40120a default_cb=0x4011e9 share window 0x401200
python3 solve.py
[*] default_cb = 0x4011e9
[*] win        = 0x40120a
[+] partial overwrite landed on win()
```

## Trying the AArch64 cross build

```text
$ make -C ch10-aslr/leak-server arm64-maybe
aarch64-linux-gnu-gcc -O1 -Wall -Wno-format-security -fno-inline \
    -fno-stack-protector -D_GNU_SOURCE -o vuln-arm64 vuln-arm64.c
$ qemu-aarch64 -L /usr/aarch64-linux-gnu ./vuln-arm64
welcome. libc=0x40008xxxxx main=0xaaaaxxxxxxxx
name?
```

Solving the AArch64 build is Exercise 6 in the chapter.
