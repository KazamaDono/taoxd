#!/usr/bin/env bash
# Compile a C source for x86-64 and AArch64 and disassemble one function
# from each, side by side.  Ubuntu 24.04: gcc 13, aarch64-linux-gnu-gcc.
# Usage: ./compare_arch.sh triangle.c triangle
set -euo pipefail
src=${1:?usage: compare_arch.sh <source.c> <function>}
fn=${2:?usage: compare_arch.sh <source.c> <function>}
out=$(mktemp -d)

gcc                 -O1 -c "$src" -o "$out/x86.o"
aarch64-linux-gnu-gcc -O1 -c "$src" -o "$out/arm.o"

echo "===================== x86-64 (System V) ====================="
objdump -d --no-show-raw-insn -M intel --disassemble="$fn" "$out/x86.o"
echo "===================== AArch64 (AAPCS64) ====================="
aarch64-linux-gnu-objdump -d --no-show-raw-insn --disassemble="$fn" "$out/arm.o"

rm -rf "$out"
