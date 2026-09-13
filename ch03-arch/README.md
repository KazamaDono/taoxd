# Chapter 3 — companion code

Architecture lab for Chapter 3, *Computer Architecture for Exploitation*. These
are **observation** artifacts, not vulnerable targets: you compile them, read
their disassembly on both ISAs, watch ASLR move their mappings, and trace the
dynamic linker rewriting a GOT slot. Everything here builds and runs on
Ubuntu 24.04 on **both** the x86-64 and the arm64 CI runner. **Lab use only** —
see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh              # drop into the Ubuntu 24.04 lab container
make -C ch03-arch test        # build everything and prove the chapter's claims
```

`make test` is the chapter's "it works" gate. It builds the native targets
(and their AArch64 cross twins), then asserts, in order:

1. `triangle` and `args8` compute the values the prose predicts (via exit status);
2. `aslr_map` prints every major region base, and those bases **freeze** under
   `setarch -R` (the ASLR-off determinism the labs rely on);
3. the x86-64 `pack` reads its 7th/8th arguments **off the stack** while the
   AArch64 `pack` reads **none** (the six- vs eight-argument-register difference);
4. `compare_arch.sh` renders `triangle` for both ISAs side by side (x86-64 host);
5. `gotplt/`: lazy binding **moves** `GOT[puts]` from the PLT resolver stub to
   libc on the first call (`trace_got.py`), the loader's own tracing shows lazy
   vs eager binding (`observe_binding.sh`), and `readelf` confirms the GOT shape
   each RELRO / `-fno-plt` build produces.

CI runs it on every push on both architectures.

## Contents

| Path | What it is |
|------|------------|
| `triangle.c` | `lst-triangle`: a sum-`1..n` loop, tiny on purpose so its codegen fits on a page. The side-by-side disassembly subject; `main` returns `triangle(100)=5050`, truncated to exit status **186**. |
| `args8.c` | `lst-args8`: an eight-`long`-argument `pack()`. On x86-64 (6 arg registers) the 7th/8th args spill to the stack; on AArch64 (8 arg registers) nothing spills. `main` returns `pack(1..8)=1401`, exit status **121**. |
| `aslr_map.c` | `lst-aslrmap`: parses `/proc/self/maps` and prints the base of each major region (PIE / libc / loader / heap / stack) plus a live stack and heap address. Run it repeatedly to watch ASLR move the bases; run under `setarch -R` to watch them freeze. |
| `disasm.sh` | Arch-aware `objdump` wrapper: disassembles one named function with symbols resolved and raw bytes hidden, Intel syntax for x86 only. Detects the target ISA from the ELF header and picks the native or `aarch64-linux-gnu-` objdump. The single-target tool Exercise 2 uses (`./disasm.sh args8.o pack`). |
| `compare_arch.sh` | `lst-compare`: compiles one C source for x86-64 and AArch64 at `-O1` and disassembles one function from each, side by side. Needs `gcc` + the `gcc-aarch64-linux-gnu` toolchain. |
| `Makefile` | Builds `triangle`/`args8`/`aslr_map` natively (`make`) and their `.arm` cross twins (`make arm`) with **explicit, commented** mitigation flags (PIE / NX / partial RELRO — modern defaults, nothing disabled, because Ch 3 *reads* present-day binaries). Provides `make test` and `make clean`. |
| `gotplt/greeter.c` | `lst-greeter`: calls `puts` twice and `printf` once — the target whose `GOT[puts]` resolution we trace. |
| `gotplt/trace_got.py` | `lst-tracegot`: pwntools + gdb-API script. Breaks at `main`, recovers the PIE base from `&main`, reads `GOT[puts]` before the first call (points into the PLT) and again after (points into libc), and **asserts the slot moved**. Arch is taken from the ELF, so it runs on x86-64 and arm64 alike. Needs `gdb`, `pwntools`, and a terminal (the Makefile runs it inside `tmux` headlessly). |
| `gotplt/observe_binding.sh` | `lst-binding`: uses `LD_DEBUG=bindings` to show `puts` bound on first call (lazy), then `LD_BIND_NOW=1` to show every symbol bound before `main` (eager). No debugger required. |
| `gotplt/Makefile` | Builds `greeter` (partial RELRO, lazy — the traced default), `greeter-fullrelro` (`-z relro -z now`), `greeter-nofplt` (`-fno-plt`), and the cross-compiled `greeter.arm` for the PLT-stub exercise. Provides `make test` and `make got` (a quick `readelf -r` GOT view). |

> The addresses in the chapter's pwndbg transcript (`lst-pwndbg-got`) are an
> illustrative ASLR-disabled run and will differ on your machine. What `make
> test` pins is the *shape* — the before/after inequality from `lst-tracegot`,
> which is reproducible — never a hardcoded address.

## Which binaries get built where

The Makefiles build whatever the host can target and run foreign-arch binaries
under `qemu-user`:

| Host | Native | Cross (AArch64) |
|------|--------|------------------|
| x86-64 | `triangle`, `args8`, `aslr_map`, `greeter*` | `*.arm` / `greeter.arm` via `aarch64-linux-gnu-gcc`, run under `qemu-aarch64-static` |
| arm64  | `triangle`, `args8`, `aslr_map`, `greeter*` (these *are* AArch64) | — (`compare_arch.sh` and the x86 side are skipped unless an x86_64 cross-gcc is installed) |

`trace_got.py` always runs against the **native** `greeter`, so the lazy-binding
proof runs on both runners. The x86-vs-ARM side-by-side (`compare_arch.sh`) runs
on the x86-64 runner; the ARM codegen it would show is instead proven on the
arm64 runner by disassembling the native `pack`.

## Expected `make test` output

On an x86-64 host (trimmed; pids/paths vary, and every base below is randomized
by ASLR between runs):

```
########## 1. native codegen computes the predicted values ##########
ok: triangle(100)=5050 -> exit 186
ok: pack(1..8)=1401 -> exit 121
########## 2. aslr_map prints every major region base ##########
PIE  text   base = 0x00005e3b7c4e9000
libc text   base = 0x00007f1c2a800000
loader text base = 0x00007f1c2ab00000
heap        base = 0x00005e3b9d1f2000
stack       base = 0x00007ffe4c7a0000
a stack variable lives at 0x00007ffe4c7be8b8
the heap block lives at   0x00005e3b9d1f22a0
########## 3. setarch -R freezes the map (ASLR-off determinism) ##########
ok: two setarch -R runs are byte-identical (ASLR disabled)
########## 4. args8 'pack': x86-64 spills to stack, AArch64 does not ##########
   mov    rax, QWORD PTR [rsp+0x8]
ok: x86-64 pack reads the 7th/8th args off the stack
ok: cross AArch64 pack uses only x0-x7 (Exercise 2 confirmed)
########## 5. compare_arch.sh side-by-side (x86-64 host only) ##########
ok: rendered triangle for x86-64 and AArch64 side by side
########## 6. optional: run the cross-built arm twins (exit codes) ##########
ok: triangle.arm -> exit 186
ok: args8.arm -> exit 121
########## 7. dynamic linking sub-lab (gotplt/) ##########
########## 1. lazy binding: GOT[puts] moves (trace_got.py) ##########
[+] GOT[puts] @ 0x...  before 1st call = 0x...036  (PLT resolver path)
[+] GOT[puts] @ 0x...  after  1st call = 0x7f...    (libc puts)
[+] lazy binding confirmed: 0x...036 -> 0x7f...
ok: GOT[puts] moved from the PLT stub to libc on first call
########## 2. loader tracing: lazy vs eager (observe_binding.sh) ##########
== default (lazy): puts is bound when first called ==
     ...: binding file ./greeter [0] to .../libc.so.6 [0]: normal symbol `puts' [GLIBC_2.2.5]
== LD_BIND_NOW=1 (eager): every symbol is bound before main ==
     ...: normal symbol `puts' [GLIBC_2.2.5]
     ...: normal symbol `printf' [GLIBC_2.2.5]
########## 3. the GOT shape each build produces (tbl-relro) ##########
-- greeter (partial RELRO, lazy): puts is a JUMP_SLOT --
ok: greeter is lazy (no BIND_NOW flag)
-- greeter-fullrelro (-z now): BIND_NOW set, eager --
-- greeter-nofplt (-fno-plt): puts via GLOB_DAT, no lazy PLT slot --
ok: -fno-plt removed the puts PLT slot
########## 4. AArch64 PLT stub adrp/ldr/br (Exercise 6) ##########
ok: greeter.arm .plt uses the adrp/ldr/br sequence (via x16/x17)
ch03-arch/gotplt: all checks passed — make test OK
ch03-arch: all checks passed — make test OK
```

`make test` exits `0` only if every check prints `ok`; any `FAIL` sets the exit
status non-zero and lists what broke.

## Doing it by hand (the chapter's workflow)

```bash
make                                   # build triangle, args8, aslr_map (native)
make arm                               # + the .arm cross twins

./compare_arch.sh triangle.c triangle  # the side-by-side of fig-disasm
./disasm.sh args8.o pack               # where x86-64 reads args 7,8 off the stack

./aslr_map; ./aslr_map                 # run twice: the bases move (ASLR on)
setarch -R ./aslr_map                  # run under no-ASLR: the bases freeze

cd gotplt && make
./observe_binding.sh                   # lazy vs eager, no debugger
python3 trace_got.py                   # watch GOT[puts] move (needs a terminal)
make got                               # readelf -r view of the GOT
```

No `.ci-skip` here: unlike the hardware-tag and Windows chapters, everything in
Chapter 3 runs on the Linux CI on both architectures.
