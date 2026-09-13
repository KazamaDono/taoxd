#!/usr/bin/env bash
# ch30-fuzzing-2/grammar/run.sh — crudest possible generation loop.
#
# Generates programs from generate.py and pipes each into the target parser.
# Any crash produces a non-empty crashes/ entry with the offending input.
# Not coverage-guided — see Exercise 7 for the libFuzzer custom-mutator
# wrapping. This is the shell equivalent of the one-liner in the chapter.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
TARGET="${HERE}/target/config_parser"
GEN="${HERE}/generate.py"
CRASHES="${HERE}/crashes"
mkdir -p "$CRASHES"

if [[ ! -x "$TARGET" ]]; then
    echo "target not built: $TARGET" >&2
    echo "run: cc -g -O1 -fsanitize=address -o $TARGET ${TARGET}.c" >&2
    exit 2
fi

iters="${1:-1000}"
for i in $(seq 1 "$iters"); do
    seed=$((RANDOM * 32768 + RANDOM))
    input="$(mktemp)"
    python3 "$GEN" "$seed" > "$input"
    if ! "$TARGET" "$input" > /dev/null 2>&1; then
        cp "$input" "$CRASHES/crash-${seed}.txt"
        echo "[crash] seed=$seed saved to $CRASHES/crash-${seed}.txt"
    fi
    rm -f "$input"
done
echo "done: $iters iterations, $(ls "$CRASHES" | wc -l) crashes"
