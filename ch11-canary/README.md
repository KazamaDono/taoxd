# Chapter 11 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 11
of *Modern Exploit Development* ("Stack Canaries, RELRO, and FORTIFY").
**Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch11-canary test    # build all sub-labs and run every exploit test
```

Every runnable sub-lab ships a `make test` that asserts the exploit reaches its
goal (writes a `pwn.marker` file, or in the RELRO-full case takes the expected
SIGSEGV). CI runs `make test` here on every push.

## Contents

| Path                                | What it is                                                                                                        |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `canary/leak.c`                     | Single-connection stdin/stdout target with a `printf(user_fmt)` leak on the same frame as the canary, then a linear `read()` overflow. |
| `canary/solve.py`                   | pwntools solve: `%N$p` sweep to extract the canary, then replay it and ret to `win()`.                            |
| `canary/Makefile`                   | Builds `leak` with `-fstack-protector-strong` (defense ON) + full RELRO + `-no-pie`; `make test` proves the pwn.  |
| `fork-brute/server.c`               | Fork-model TCP server on `127.0.0.1:4011`; child handler `recv()`s 200 bytes into a 64-byte struct and acks `"ok\n"`. |
| `fork-brute/solve.py`               | Byte-by-byte canary brute over the socket (Chapter 11's lst-brute is this file's `brute_canary()` verbatim) plus the follow-up hijack. |
| `fork-brute/Makefile`               | Builds `server`, provides `make run` (foreground) and `make test` (CI-driven: starts the server, runs the solve, checks the marker). |
| `relro/relro_demo.c`                | One source with a scripted arbitrary-write primitive that targets `puts@got.plt`.                                 |
| `relro/solve.py`                    | Points `puts@got.plt` at `win()`; succeeds against `relro_partial`, faults against `relro_full`.                  |
| `relro/Makefile`                    | Builds `relro_partial` (`-Wl,-z,relro`) and `relro_full` (`-Wl,-z,relro,-z,now`) from the same source and asserts both outcomes. |
| `fortify/demo.c`                    | The lst-fortify listing verbatim: `copy_static`, `copy_dynamic`, `copy_manual`.                                   |
| `fortify/Makefile`                  | Builds `demo` three times (`_FORTIFY_SOURCE`=1, 2, 3) and diffs `copy_dynamic`'s disassembly to show where `__strcpy_chk` first appears. |
| `test.sh`                           | CI entrypoint: runs each sub-lab's `make test` in a fixed order and prints a single OK banner on success.        |
| `Makefile`                          | Top-level: `make test` → `bash test.sh`.                                                                          |

## Expected `make test` output (abridged)

```
===================================================================
 ch11-canary: build + test all sub-labs
===================================================================

 >>> canary
== canary leak + reproduction ==
CANARY-LEAK-OK
ch11-canary/canary: OK

 >>> fork-brute
== starting fork-brute server on 127.0.0.1:4011 ==
== brute + reproduce canary + hijack ret ==
FORK-BRUTE-OK
ch11-canary/fork-brute: OK

 >>> relro
== [1/2] relro_partial : expect hijack success ==
RELRO-PARTIAL-OK: puts@got hijack succeeded
== [2/2] relro_full    : expect SIGSEGV on the write ==
RELRO-FULL-OK: write faulted as expected (sig=11)
ch11-canary/relro: OK

 >>> fortify
== Disassembly of copy_dynamic at each FORTIFY level ==
--- _FORTIFY_SOURCE=1 ---
    ...  callq  <strcpy@plt>
--- _FORTIFY_SOURCE=2 ---
    ...  callq  <strcpy@plt>
--- _FORTIFY_SOURCE=3 ---
    ...  callq  <__strcpy_chk@plt>
== positive control: copy_static must be fortified at _F_S=1 ==
== main claim: copy_dynamic must be fortified at _F_S=3 ==
ch11-canary/fortify: OK

===================================================================
 ALL-CH11-TESTS-OK
===================================================================
```

## Requirements

Ubuntu 24.04 (glibc 2.39, gcc 13, binutils 2.42), `python3`, `pwntools`,
`setarch`, `objdump`. All present in the pinned lab image. The AArch64 build
of `canary/leak` is optional and only fires if `aarch64-linux-gnu-gcc` and
`qemu-aarch64` are available (`make -C canary arm64`).
