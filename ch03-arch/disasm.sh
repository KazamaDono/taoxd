#!/usr/bin/env bash
# disasm.sh - arch-aware objdump wrapper: disassemble ONE named function with
# symbols resolved and raw opcode bytes hidden.  Intel syntax for x86 targets;
# AArch64 has a single disassembly syntax so none is forced there.  Works on
# object files and fully linked PIEs alike.  Ubuntu 24.04 / binutils 2.42.
#
#   Usage: ./disasm.sh <file.o|binary> <function>
#   e.g.:  ./disasm.sh args8.o     pack      # x86-64  -> reads args 7,8 off stack
#          ./disasm.sh args8-arm.o pack      # AArch64 -> all 8 args in registers
#
# This is the single-target companion the exercises reach for; compare_arch.sh
# runs the equivalent objdump invocation directly for the side-by-side build.
set -euo pipefail
file=${1:?usage: disasm.sh <object|binary> <function>}
fn=${2:?usage: disasm.sh <object|binary> <function>}
[ -r "$file" ] || { echo "disasm.sh: cannot read '$file'" >&2; exit 1; }

# --- detect the target ISA from the ELF header -----------------------------
machine=$(readelf -h "$file" 2>/dev/null \
            | sed -n 's/^[[:space:]]*Machine:[[:space:]]*//p')

# pick the first tool on PATH from the candidates (fallback: the first name)
pick() { local c; for c in "$@"; do
           command -v "$c" >/dev/null 2>&1 && { echo "$c"; return; }
         done; echo "$1"; }

case "$machine" in
  *AArch64*)
    OBJDUMP=$(pick aarch64-linux-gnu-objdump objdump)
    SYNTAX=()                      # AArch64 disassembly has one syntax
    ;;
  *X86-64*)
    OBJDUMP=$(pick objdump x86_64-linux-gnu-objdump)
    SYNTAX=(-M intel)              # Intel syntax only for x86, per the chapter
    ;;
  *)
    echo "disasm.sh: unrecognized machine '$machine' in '$file'" >&2
    exit 1
    ;;
esac

# -d                  disassemble
# --disassemble=FN    just this one function (binutils >= 2.34)
# --no-show-raw-insn  hide the raw opcode bytes, leaving only the mnemonics
exec "$OBJDUMP" -d --no-show-raw-insn "${SYNTAX[@]}" --disassemble="$fn" "$file"
