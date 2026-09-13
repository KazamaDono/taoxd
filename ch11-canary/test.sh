#!/usr/bin/env bash
# ch11-canary/test.sh -- CI entrypoint. Runs each sub-lab's `make test` in a
# fixed order and asserts the expected outcomes:
#
#   canary      : format-string leak + linear overflow across reproduced canary
#                 -> win() writes pwn.marker.
#   fork-brute  : byte-by-byte canary brute against the server, then a
#                 follow-up connection that hijacks the return address.
#   relro       : the SAME arbitrary-write primitive succeeds against
#                 relro_partial and SIGSEGVs against relro_full.
#   fortify     : demo.c built three times; disassembly of copy_dynamic
#                 gains __*_chk at _FORTIFY_SOURCE=3.
#
# Exits 0 only if every step's marker/assertion is satisfied. Deterministic:
# ASLR is disabled inside each sub-test's runner via `setarch -R`.

set -euo pipefail
cd "$(dirname "$0")"

echo "==================================================================="
echo " ch11-canary: build + test all sub-labs"
echo "==================================================================="

for sub in canary fork-brute relro fortify; do
    echo
    echo "-------------------------------------------------------------------"
    echo " >>> $sub"
    echo "-------------------------------------------------------------------"
    make -C "$sub" test
done

echo
echo "==================================================================="
echo " ALL-CH11-TESTS-OK"
echo "==================================================================="
