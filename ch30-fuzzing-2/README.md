# Chapter 30 — Fuzzing II: Structure-Aware and Snapshot Fuzzing

Companion code for Chapter 30 of *Modern Exploit Development*.
**Lab use only** — see `../ETHICS.md`.

Every listing in the chapter maps to a real file below; each planted bug is
reachable by the technique the section teaches. The `Makefile`'s `test`
target compiles the deliberately-vulnerable C targets with ASan and asserts
each planted bug trips a sanitizer — this is what CI runs on every push.

## Build & run

```bash
# from the repo root
./scripts/lab.sh                       # drop into the Ubuntu 24.04 lab
make -C code/ch30-fuzzing-2 test       # deterministic bug reproducers + ASan
```

Heavier fuzz campaigns (opt-in, need external libraries):

```bash
make -C code/ch30-fuzzing-2 fuzz-fdp        # FuzzedDataProvider libFuzzer harness
make -C code/ch30-fuzzing-2 fuzz-lpm        # libprotobuf-mutator harness
make -C code/ch30-fuzzing-2 fuzz-grammar    # shell-driver grammar loop
make -C code/ch30-fuzzing-2 fuzz-diff       # jsoncpp vs. yyjson differential
```

## Contents

| Path | What it is | Listing |
|------|------------|---------|
| `structured/fdp_harness.cc` | FuzzedDataProvider libFuzzer harness for `parse_record`. | `lst-fdp` |
| `structured/target.c` | Toy `parse_record` with a planted version-3 memcpy overflow. | — |
| `structured/config.proto` | Protobuf schema for the LPM-driven config fuzzer. | `lst-lpm-proto` |
| `structured/lpm_harness.cc` | LPM entry point + emitter turning `Config` messages into text. | `lst-lpm-harness` |
| `structured/parse_config.c` | Toy recursive config-text parser with a planted stack overflow. | — |
| `structured/CMakeLists.txt` | Build wiring for both structure-aware harnesses. | — |
| `grammar/generate.py` | Weighted recursive-descent grammar generator with a depth budget. | `lst-grammar` |
| `grammar/target/config_parser.c` | Toy parser for the generated language; planted identifier overflow. | — |
| `grammar/run.sh` | Crudest possible generation loop: `generate.py | target` in a shell. | — |
| `snapshot/README.md` | Nyx-style snapshot-fuzz walkthrough against the ch24 VM. | — |
| `snapshot/agent.c` | Minimal guest agent (observer channel + entropy stubs). | — |
| `syzkaller/ch24_lkm.txt` | syzlang description for `/dev/vuln` (stack overflow + KASLR leak). | `lst-syzlang` |
| `syzkaller/config.cfg` | `syz-manager` config template for the ch24 kernel + a Noble rootfs image. | — |
| `syzkaller/reproduce.md` | How to run `syz-manager`, read coverage, minimize reproducers. | — |
| `differential/json_diff.cc` | Differential libFuzzer harness across jsoncpp and yyjson. | `lst-diff` |
| `differential/seed_corpus/` | Curated JSON seeds for the differential harness. | — |
| `Makefile` | Top-level targets: `build-all`, `test`, `fuzz-{fdp,lpm,grammar,diff}`. | — |

## Expected `make test` output

The exact ASan stack lines vary with libc / clang builds; the structural
pattern is what CI asserts:

```
==== ch30-fuzzing-2 ====
[test] structured/target.c :: planted heap/stack overflow
=================================================================
==NNN==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x...
  WRITE of size 196 at 0x... thread T0
    #0 memcpy ...
    #1 parse_record structured/target.c:...
PASS: ASan tripped the planted overflow
[test] structured/parse_config.c :: unbounded recursion
==NNN==ERROR: AddressSanitizer: stack-overflow on address 0x...
PASS: recursion tripped the sanitizer
[test] grammar/target/config_parser.c :: identifier overflow
==NNN==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x...
PASS: ASan tripped the planted overflow
[test] grammar/generate.py :: deterministic output
PASS: seed=42 stable output (NNN bytes)
[test] snapshot/agent.c :: builds and runs
[agent] hypercall cmd=0x01 (stubbed)
[agent] input page @ gpa=0x00000000, tsc=0, rand=0
[agent] hypercall cmd=0x02 (stubbed)
MED_CH30_SNAPSHOT_AGENT_OK
PASS: agent handshake stub ran
[test] libFuzzer harness syntax check (best-effort)
PASS: fdp_harness.cc parses under clang-18
==== ch30-fuzzing-2: all tests PASSED ====
```

## Mitigation flags — what's on and why

Deliberately-vulnerable C targets in this directory are built with:

- `-fsanitize=address,undefined` — ASan is the oracle every planted bug
  trips. UBSan catches sign-conversion arithmetic bugs a grammar mutator
  can wander into.
- `-fno-omit-frame-pointer` — readable ASan backtraces during triage.
- `-fstack-protector-strong` — **ON as a safety net**. The planted bug in
  `structured/target.c` is a forward `memcpy` into a fixed-size buffer,
  which writes past the buffer's end but does not cross the canary
  contiguously; ASan's redzone detection is what fires, not SSP.
- No `-D_FORTIFY_SOURCE=3` — FORTIFY would rewrite `memcpy` at compile
  time and mask the exact bug class this chapter demonstrates.
- No `-pie -fPIE` — these are hosted test doubles, not exploitation targets.

`ASAN_OPTIONS='abort_on_error=0:exitcode=42'` is set inside each `_test_*`
recipe so the test harness's `grep` for the ASan marker is what decides
pass/fail, not the OS's disposition of `SIGABRT`.
