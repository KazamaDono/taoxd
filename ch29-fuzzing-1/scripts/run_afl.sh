#!/usr/bin/env bash
# Ubuntu 24.04, AFL++ 4.21c, clang-18. Run from ch29-fuzzing-1/.
set -euo pipefail

export AFL_USE_ASAN=1                                                   # ❶
export AFL_SKIP_CPUFREQ=1 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

# ❷ Build instrumented target with the AFL++ clang wrapper. The wrapper
#    inserts edge counters and the fork-server stub so afl-fuzz can drive it.
mkdir -p build
AFL_CC=clang-18 afl-clang-fast -g -O1 -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Isrc \
    src/csv_parse.c harness/fuzz_csv.c \
    "$(afl-config --libdir 2>/dev/null)/libAFLDriver.a" \
    -o build/fuzz_csv_afl

DUR="${AFL_DURATION:-300}"
mkdir -p findings
timeout "$DUR" afl-fuzz -i corpus/seeds -o findings -x corpus/csv.dict \
    -M primary   -- ./build/fuzz_csv_afl @@ &                           # ❸
timeout "$DUR" afl-fuzz -i corpus/seeds -o findings -x corpus/csv.dict \
    -S secondary -l 2 -- ./build/fuzz_csv_afl @@ &                      # ❹
wait

ls findings/*/crashes/ | head                                           # ❺
