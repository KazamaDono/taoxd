#!/usr/bin/env bash
# Invoke the pinned d8 with the flags the chapter uses, load vuln.js,
# addrof-fakeobj.js, helpers.js, rw.js in order, then run the smoke test.
# Deterministic: setarch -R disables ASLR in this process only.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D8="${D8:-$HERE/d8}"

if [[ ! -x "$D8" ]]; then
  echo "[run.sh] pinned d8 not found at $D8" >&2
  echo "[run.sh] build it first: bash $HERE/build.sh" >&2
  exit 2
fi

# --allow-natives-syntax    lets us use %PrepareFunctionForOptimization etc
# --expose-gc               lets the smoke test call gc() for the survival check
# --no-lazy-feedback-allocation  make feedback vectors eager => Maglev tier-up is deterministic
FLAGS=(--allow-natives-syntax --expose-gc --no-lazy-feedback-allocation)

# setarch -R: no ASLR for this process; keeps offsets stable when a run
# is debugged interactively.
exec setarch -R "$D8" "${FLAGS[@]}" \
  "$HERE/exploit/vuln.js" \
  "$HERE/exploit/addrof-fakeobj.js" \
  "$HERE/exploit/helpers.js" \
  "$HERE/exploit/rw.js" \
  "$HERE/test/smoke.js"
