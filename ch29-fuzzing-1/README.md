# Chapter 29 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 29
of *Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                          # drop into the Ubuntu 24.04 lab container
make -C code/ch29-fuzzing-1 test          # build the replay harness + prove the ASan oracle
make -C code/ch29-fuzzing-1 fuzz          # optional: 100k libFuzzer runs against the seed corpus
bash code/ch29-fuzzing-1/scripts/run_afl.sh    # optional: full AFL++ campaign (needs AFL++ 4.x)
bash code/ch29-fuzzing-1/scripts/coverage.sh   # optional: llvm-cov HTML + terminal report
```

`make test` is the CI-runnable gate. It builds a non-fuzzer replay binary
under `-fsanitize=address,undefined`, feeds every seed through it (all must
parse cleanly), then feeds `corpus/crash_seeds/uaf.bin` through it and
requires AddressSanitizer to report a `heap-use-after-free` with exit
code 42. The `fuzz` and `run_afl.sh` targets are the interactive campaigns
described in the chapter; they are not run in CI because a real campaign
needs minutes-to-hours to be useful.

## Contents

| Path | What it is |
|------|------------|
| `src/csv_parse.h` | Public interface for the vulnerable CSV parser fuzzed in this chapter. |
| `src/csv_parse.c` | Tiny CSV parser with three planted bugs: an integer-driven heap overflow, an off-by-one write on doubled-quote escapes, and a heap-use-after-free triggered by the `\z` escape. |
| `harness/fuzz_csv.c` | Single `LLVMFuzzerTestOneInput` harness that drives both libFuzzer and AFL++'s `libAFLDriver` unchanged. |
| `harness/replay_main.c` | Non-fuzzer `main()` used by the coverage build and `make test` to replay one corpus file through the harness. |
| `corpus/csv.dict` | AFL-format dictionary of CSV structural tokens (separator, quote, escapes, BOM). |
| `corpus/seeds/hello.csv` | Small valid CSV seed exercising quoted fields and doubled-quote escapes. |
| `corpus/seeds/minimal.csv` | Three unquoted fields on one line. |
| `corpus/seeds/quoted.csv` | Quoted fields with a `\n` escape. |
| `corpus/crash_seeds/uaf.bin` | Four-byte reproducer for BUG 3 (`A\zB`) that fires the planted `heap-use-after-free`. |
| `scripts/run_afl.sh` | End-to-end AFL++ campaign script: build with `afl-clang-fast` + ASan, run primary and CMPLOG secondary. |
| `scripts/coverage.sh` | Build a coverage replay binary, run the corpus through it, produce `llvm-cov` HTML and terminal reports. |
| `scripts/triage.py` | Group crash inputs by ASan top-3-frame stack signature to deduplicate a campaign's crashes. |
| `Makefile` | Build targets for the replay binary, the optional libFuzzer harness, and the CI `test` target. |

## Expected `make test` output

The test target prints, roughly:

```
== ch29 fuzzing lab test ==
-- compiler: /usr/bin/clang-18
-- replay seed  corpus/seeds/hello.csv
-- replay seed  corpus/seeds/minimal.csv
-- replay seed  corpus/seeds/quoted.csv
-- replay crash corpus/crash_seeds/uaf.bin (expect ASan UAF)
=================================================================
==NNN==ERROR: AddressSanitizer: heap-use-after-free on address 0x... at pc 0x...
WRITE of size 1 at 0x... thread T0
    #0 0x... in csv_parse_row src/csv_parse.c:...
    #1 0x... in LLVMFuzzerTestOneInput harness/fuzz_csv.c:...
    #2 0x... in main harness/replay_main.c:...
...
[ch29] all fuzzing checks passed
```

The final `[ch29] all fuzzing checks passed` line is the unique success
marker CI greps for. Any other outcome — a seed that unexpectedly aborts,
a missing UAF report, or a wrong exit code — fails the build.

## The planted bugs

| ID | Class | Trigger idea | Sanitizer verdict |
|----|-------|--------------|-------------------|
| 1 | Heap overflow via integer wrap | Fill one field with > 32 KiB of non-delimiter bytes so the `unsigned short` capacity wraps. | `AddressSanitizer: heap-buffer-overflow WRITE` |
| 2 | Off-by-one write on `""` escape | Exactly fill the field buffer inside quotes, then close with `""`. | `AddressSanitizer: heap-buffer-overflow WRITE of size 1` |
| 3 | Use-after-free on `\z` escape | Any byte after `\z` inside an unquoted field. `corpus/crash_seeds/uaf.bin` is the four-byte reproducer. | `AddressSanitizer: heap-use-after-free WRITE of size 1` |

The chapter walks through discovering each with a live fuzz campaign;
`make test` only asserts on bug 3 because it is the cheapest to trigger
deterministically without letting a fuzzer run.
