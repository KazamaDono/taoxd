#!/usr/bin/env bash
# build-all.sh — `make` every chapter that has a Makefile (skips .ci-skip dirs).
set -uo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"
rc=0
for dir in ch*/; do
  dir="${dir%/}"
  [[ -f "$dir/.ci-skip" ]] && { echo "SKIP  $dir"; continue; }
  [[ -f "$dir/Makefile" ]] || { echo "----  $dir (no Makefile)"; continue; }
  echo "==== build $dir ===="
  make -C "$dir" || { echo "BUILD FAIL $dir"; rc=1; }
done
exit $rc
