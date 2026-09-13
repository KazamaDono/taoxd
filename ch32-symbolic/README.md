# Chapter 32 — companion code

Deliberately vulnerable teaching artifacts and working angr scripts for
Chapter 32 (*Symbolic and Concolic Execution*) of *Modern Exploit
Development*. **Lab use only** — see `../ETHICS.md`.

Toolchain (pinned): Ubuntu 24.04, glibc 2.39, gcc 13, clang 18,
Python 3.12, **angr 9.2.x** (see `requirements.txt`).

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                  # drop into the Ubuntu 24.04 lab container
make -C ch32-symbolic test        # build targets, install angr, run pytest
```

## Contents

| Path | What it is |
|------|------------|
| `src/xorcheck.c` | Crackme: length gate + S-box + XOR-feedback (Listing 32-1 excerpt). Public `win`/`lose` symbols for angr to resolve by name. |
| `src/parser.c` | Length-prefixed record parser with a signed/unsigned bug at `do_copy()` (int32 length cast to `size_t`). |
| `solve/solve_xorcheck.py` | Listing 32-2 verbatim (plus argv). Recovers the crackme key via `explore(find=win, avoid=[lose])` with printable-ASCII constraints. |
| `solve/find_crash.py` | Listing 32-4 verbatim (plus argv). Marks stdin symbolic and synthesizes an input that reaches `do_copy`. |
| `concolic/flip_branch.py` | Listing 32-3 verbatim (plus CLI). Concolic driver: pin stdin to a concrete seed, walk with `Tracer`, drop the equality at a chosen state, re-solve. |
| `concolic/extract_primitives.py` | Iterates `simgr.unconstrained` and classifies each state (pc-control / write-what-where / neither). Basis for Exercise 4. |
| `tests/test_solvers.py` | pytest: solver output round-trips through the real binary, and the crash finder produces a segfaulting input. |
| `Makefile` | Builds x86-64 and (optionally) arm64 targets; `make test` runs the whole rig. |
| `requirements.txt` | angr 9.2.x + claripy + pytest, pinned for Python 3.12. |

## Expected `make test` output (abridged)

```
gcc -O2 -std=c11 -Wall -Wextra -fno-inline -fPIE -pie -fstack-protector-strong \
    -Wl,-z,relro,-z,now,-z,noexecstack -o xorcheck-x86_64 src/xorcheck.c
gcc -O0 -g  -std=c11 ... -fno-stack-protector -U_FORTIFY_SOURCE ...  -o parser-x86_64 src/parser.c
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pytest -q tests/test_solvers.py
..                                                                       [100%]
2 passed in ~40s
```

The two passing tests correspond to:

1. `test_xorcheck_solver_recovers_valid_key` — runs
   `solve_xorcheck.py`, extracts the printed key, feeds it back to
   `./xorcheck-x86_64`, and asserts the binary prints the
   `SOLVED` marker and exits 0.
2. `test_find_crash_produces_segfaulting_input` — runs
   `find_crash.py`, pipes the emitted bytes into `./parser-x86_64`,
   and asserts the process is killed by a signal (SIGSEGV from the
   negative-length `memcpy`).

## AArch64 (Exercise 3)

`make arm64` cross-builds the same sources with `aarch64-linux-gnu-gcc`.
The solver scripts are architecture-agnostic — angr reads the ISA from
the ELF header:

```bash
make arm64
./solve/solve_xorcheck.py ./xorcheck-arm64
```

## Mitigation flag notes

Both targets are built PIE + full RELRO + NX. `xorcheck` keeps the
stack protector on — the point is that neither stack canaries nor
ASLR obstruct a solver's reasoning. `parser` disables the stack
protector and FORTIFY so the vulnerable `memcpy` at `do_copy` actually
runs (otherwise glibc's fortified copy or the canary would abort
before the overflow, and the "reachable" answer would be uninteresting).
Each choice is annotated inline in the `Makefile`.
