#!/usr/bin/env bash
# test-all.sh — run `make test` in every chapter that supports it.
# A chapter opts out of CI by creating a `.ci-skip` file (Windows/HW-only labs);
# the reason in that file is printed. Run from the repo root, inside the lab.
set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

pass=0; fail=0; skip=0; failed_dirs=()

for dir in ch*/; do
  dir="${dir%/}"
  if [[ -f "$dir/.ci-skip" ]]; then
    echo "SKIP  $dir — $(cat "$dir/.ci-skip")"
    skip=$((skip+1)); continue
  fi
  if [[ ! -f "$dir/Makefile" ]] || ! grep -qE '^test:' "$dir/Makefile"; then
    echo "SKIP  $dir — no test target"
    skip=$((skip+1)); continue
  fi
  echo "==== $dir : make test ===="
  if make -C "$dir" test; then
    echo "PASS  $dir"; pass=$((pass+1))
  else
    echo "FAIL  $dir"; fail=$((fail+1)); failed_dirs+=("$dir")
  fi
done

echo
echo "================ summary ================"
echo "  PASS: $pass   FAIL: $fail   SKIP: $skip"
if (( fail > 0 )); then
  printf '  failed: %s\n' "${failed_dirs[*]}"
  exit 1
fi
echo "  all runnable chapter tests passed."
