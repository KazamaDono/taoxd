#!/usr/bin/env bash
# recon.sh — everything you should know about a target before writing exploits.
set -euo pipefail
BIN=${1:-./target/notesd}

echo "== file =="                                              # ❶
file "$BIN"
echo
echo "== checksec (mitigations) =="                            # ❷
pwn checksec "$BIN" 2>&1 | grep -E 'RELRO|Stack|NX|PIE|Fortify|CET'
echo
echo "== strings (interesting) =="                             # ❸
strings -n 6 "$BIN" | grep -Ei 'usage|version|error|/bin|token' | head
echo
echo "== imports =="                                           # ❹
objdump -T "$BIN" | awk '{print $NF}' | sort -u | grep -E '^(read|recv|memcpy|strcpy|malloc|system|execve)$' | head
echo
echo "== ldd =="                                               # ❺
ldd "$BIN"
