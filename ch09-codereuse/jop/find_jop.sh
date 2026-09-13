#!/usr/bin/env bash
# List indirect-branch gadgets (JOP/COP material) in a binary.
set -euo pipefail
bin="${1:?usage: find_jop.sh <binary>}"

echo "== dispatcher candidates (advance + indirect jmp) =="
ropper --file "$bin" --type jop 2>/dev/null | grep -E 'jmp (qword )?\[?r' | head -40

echo "== call-oriented gadgets =="
ROPgadget --binary "$bin" --only "pop|call|jmp" 2>/dev/null \
    | grep -E ': call ' | head -40
