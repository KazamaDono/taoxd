# Chapter 2 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 2 of *Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

This chapter stands up the lab the rest of the book runs in. The shared
infrastructure it introduces — the pinned image, the compose file, and the
toolchain verifier — lives at the repo root and is used by every chapter:

| Path | What it is |
|------|------------|
| [`../lab/Dockerfile`](../lab/Dockerfile) | Listing 2-1 — the pinned Ubuntu 24.04 lab image (glibc 2.39, gcc 13, clang 18, gdb+pwndbg, AArch64 cross-toolchain, `qemu-user`/`qemu-system`, pwntools/ROPgadget/one_gadget). |
| [`../lab/docker-compose.yml`](../lab/docker-compose.yml) | Listing 2-2 — grants only `SYS_PTRACE` and `seccomp=unconfined` (for gdb and `setarch -R`) and bind-mounts the repo at `/work`. |
| [`../scripts/doctor.sh`](../scripts/doctor.sh) | Listing 2-3 — `make doctor`; probes each tool and prints its version. |
| [`../scripts/lab.sh`](../scripts/lab.sh) | One-command wrapper to build/enter the pinned lab container. |

The chapter's own smoke-test target and its end-to-end exploit live here:

| Path | What it is |
|------|------------|
| [`hello_overflow.c`](hello_overflow.c) | Listing 2-4 — the smoke-test target: a 64-byte buffer overflowed by `read(0, buf, 256)`, with a never-called `win()` sink that prints a token via raw `write()`/`_exit()`. Insecure on purpose. |
| [`Makefile`](Makefile) | Listing 2-5 — builds `hello_overflow` (native x86-64) and `hello_overflow_arm64` (`aarch64-linux-gnu`, `-static`) with annotated teaching flags; `make test` runs the exploit against both arches. |
| [`solve.py`](solve.py) | Listing 2-6 — pwntools end-to-end proof: overflows the buffer, redirects into `win()`, and asserts the `CH02_LAB_OK` token. Runs amd64 natively and aarch64 via `qemu-aarch64-static`. |
| `README.md` | This file. |

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch02-lab test        # build the target(s) and run the exploit test
```

Every runnable lab ships a `make test` that asserts the exploit reaches its goal; CI runs it on every push.

## Teaching flags

`hello_overflow` is compiled **deliberately insecure** so the overflow reaches
the return address at all — Chapter 5 turns every one of these back on:

- `-fno-stack-protector` — no stack canary, so the linear overflow runs straight over the saved return address.
- `-no-pie` — fixed load address, so `win()` has a stable address the exploit can name.
- `-g -O0` — unoptimized, with debug info, for clean single-stepping; `-w` silences the intended-bug warnings.

The AArch64 build adds `-static` so `qemu-user` needs no guest sysroot to find
shared libraries.

## Expected `make test` output

```text
/work/ch02-lab$ make test
python3 solve.py amd64
[+] amd64: control at offset 72 -> win() reached
python3 solve.py aarch64
[+] aarch64: control at offset 80 -> win() reached
```

`make test` builds both binaries and then runs `solve.py` for each
architecture; it **exits 0 only if both reach `win()`**, and that exit status
is exactly what `scripts/test-all.sh` and CI assert. The two offsets (72 on
x86-64, 80 on the CI AArch64 build) are *discovered* by a short bounded search,
not hardcoded, so they stay correct across compiler-padding differences and
between native execution and `qemu-user`.

## Notes

- `solve.py` reads the target's architecture from the ELF itself
  (`context.binary = ELF(path)`), so the same script packs addresses correctly
  for both binaries and runs unchanged on an x86-64 or an AArch64 host.
- The token is emitted with raw `write()`/`_exit()` rather than
  `system("/bin/sh")` so reaching `win()` succeeds regardless of stack
  alignment — which keeps the smoke test deterministic in CI.
