# Chapter 31 — Static Analysis and Variant Analysis

Companion code for Chapter 31 of *Modern Exploit Development*. Seeded C
files, three static-analysis rules (Semgrep, weggli, CodeQL), and a
variant-hunt driver script. **Lab use only** — see `../ETHICS.md`.

Pinned toolchain: Ubuntu 24.04, gcc 13, glibc 2.39, Semgrep 1.90,
weggli 0.2.4, CodeQL CLI 2.17.

## Build & run

```bash
make -C ch31-static-variant test        # compile lab + run the Semgrep rule
```

`make test` compiles every translation unit in `target/` and `variant/`
and then runs the Semgrep rule from Listing 31-1 against `target/`.
Semgrep is installed on demand into `.venv` if it is not already on
`PATH`. `weggli` and `codeql` are exercised by
`variant/run-variant.sh`; the test recipe uses them opportunistically if
they are on `PATH` and skips them (with a printed note) otherwise, since
CodeQL in particular is too heavy to install inside every CI run.

## Contents

| Path                                    | What it is                                                                        |
|-----------------------------------------|-----------------------------------------------------------------------------------|
| `Makefile`                              | Build + test entry point. Comments spell out mitigation flags on the lab objects. |
| `target/proto.h`                        | Shared protocol declarations (`hdr_t`, `session_t`, function prototypes).         |
| `target/net.c`                          | Network taint source (`recv` into the header buffer).                             |
| `target/parser.c`                       | Three memcpy call sites: two bounded, one not. Semgrep target.                    |
| `target/handler.c`                      | Network-tainted memcpy (CodeQL sink) plus a UAF in `session_close` (weggli hit).  |
| `semgrep-rules/unchecked-memcpy.yaml`   | Listing 31-1 verbatim.                                                            |
| `weggli-patterns/uaf.txt`               | Listing 31-2 verbatim.                                                            |
| `codeql/qlpack.yml`                     | CodeQL pack manifest (depends on `codeql/cpp-all`).                               |
| `codeql/queries/NetworkMemcpy.ql`       | Listing 31-3 verbatim.                                                            |
| `variant/run-variant.sh`                | Listing 31-4 verbatim — end-to-end variant hunt driver.                           |
| `variant/admin.c`                       | Seeded variant #1 (length prefix inside packet).                                  |
| `variant/cache.c`                       | Seeded variant #2 (length inside header trailer).                                 |
| `variant/stats.c`                       | False-positive variant — bounded via helper function.                             |
| `variant/CVE-LAB-0001.md`               | Lab-only writeup of the seed bug.                                                 |

## Expected `make test` output

```
gcc -Wall ... -c target/net.c -o target/net.o
gcc -Wall ... -c target/parser.c -o target/parser.o
gcc -Wall ... -c target/handler.c -o target/handler.o
gcc -Wall ... -c variant/admin.c -o variant/admin.o
gcc -Wall ... -c variant/cache.c -o variant/cache.o
gcc -Wall ... -c variant/stats.c -o variant/stats.o
[ch31] using semgrep at: /usr/local/bin/semgrep      (or .venv/bin/semgrep)
1.90.0
----- semgrep.out -----
target/parser.c
    parse_record  unchecked-memcpy-fixed-dest
        memcpy(buf, hdr->data, hdr->len);
-----------------------
[ch31] semgrep OK — 1 finding(s) in target/
[ch31] weggli not installed — skipping UAF sweep (see run-variant.sh)
[ch31] codeql not installed — skipping (see variant/run-variant.sh)
ch31 test: PASSED
```

The one true-positive is the unbounded `memcpy` in `parse_record`; the
two bounded call sites above it are correctly excluded by the
`pattern-not` clauses in the rule. Follow the exercises at the end of
the chapter to extend the rule set and to drive the CodeQL variant
sweep with `variant/run-variant.sh`.
