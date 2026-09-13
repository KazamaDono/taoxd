#!/usr/bin/env bash
set -euo pipefail
# Non-fuzzer replay binary: same harness, source-based coverage, no libFuzzer.
mkdir -p build
clang-18 -g -O0 -fprofile-instr-generate -fcoverage-mapping \
    -DREPLAY_MAIN -Isrc src/csv_parse.c harness/fuzz_csv.c \
    harness/replay_main.c -o build/replay                               # ❶

rm -f build/*.profraw
for f in corpus/seeds/* corpus/crash_seeds/* \
         $(ls findings/*/queue/* 2>/dev/null || true); do
    [ -f "$f" ] || continue                                             # ❷
    LLVM_PROFILE_FILE="build/%p.profraw" ./build/replay "$f" || true
done
llvm-profdata-18 merge -sparse build/*.profraw -o build/merged.profdata
llvm-cov-18 show build/replay -instr-profile=build/merged.profdata \
    -format=html -output-dir=build/cov src/csv_parse.c                  # ❸
llvm-cov-18 report build/replay -instr-profile=build/merged.profdata \
    src/csv_parse.c                                                     # ❹
