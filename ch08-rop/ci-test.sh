#!/usr/bin/env bash
# ch08-rop/ci-test.sh - build the lab targets and prove the exploits.
# Exits 0 ONLY when the hard gates pass. Called by `make test`.
#
# Hard gates:
#   * x86-64 host: the x86-64 ret2libc AND the stack-pivot demo must pop a
#     shell (asserted via the unique PWNED marker). The AArch64 lane is run
#     best-effort under qemu.
#   * aarch64 host: the AArch64 target must build and run (smoke gate); the
#     full AArch64 ret2libc is attempted and reported but is best-effort,
#     because it was authored without a local aarch64 box to verify against.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
LANE="${1:-all}"
HOST="$(uname -m)"
MARKER='shell confirmed'
TIMEOUT="${TIMEOUT:-90}"
export PWNLIB_NOTERM=1 PWNLIB_SILENT=0

x86_buildable() { gcc -dumpmachine 2>/dev/null | grep -q x86_64; }
arm_cc() { if [ "$HOST" = aarch64 ]; then echo gcc; else command -v aarch64-linux-gnu-gcc; fi; }
qemu_arm() { command -v qemu-aarch64-static || command -v qemu-aarch64; }

ran_hard=0
rc=0

run_script() { # dir script -> 0 on success + marker
  local dir="$1" script="$2" pre="${3:-}"
  ( cd "$dir" && timeout "$TIMEOUT" $pre python3 "$script" ) 2>&1 | tee /tmp/ch08.$$
  local st=${PIPESTATUS[0]}
  grep -q "$MARKER" /tmp/ch08.$$; local g=$?
  rm -f /tmp/ch08.$$
  [ "$st" -eq 0 ] && [ "$g" -eq 0 ]
}

do_x86() {
  echo "==== build x86-64 targets ===="
  make -C target vuln-x86_64 vuln_pivot-x86_64 || return 1
  cp -f target/vuln-x86_64 target/vuln_pivot-x86_64 exploit/
  ran_hard=1
  echo "==== x86-64 ret2libc (solve_x86_64.py) ===="
  if run_script exploit solve_x86_64.py "setarch -R"; then
    echo "PASS  x86-64 ret2libc"
  else
    echo "FAIL  x86-64 ret2libc"; rc=1
  fi
  echo "==== x86-64 stack pivot (pivot_demo.py) ===="
  if run_script exploit pivot_demo.py "setarch -R"; then
    echo "PASS  x86-64 stack pivot"
  else
    echo "FAIL  x86-64 stack pivot"; rc=1
  fi
}

do_arm() {
  local acc qemu hard=0
  acc="$(arm_cc)"
  [ "$HOST" = aarch64 ] && hard=1
  if [ -z "$acc" ]; then
    echo "SKIP  AArch64 lane — no aarch64 compiler on this host"
    return 0
  fi
  echo "==== build AArch64 target ===="
  if ! make -C target vuln-arm64; then
    if [ "$hard" = 1 ]; then echo "FAIL  AArch64 build"; rc=1; return 1; fi
    echo "WARN  AArch64 build failed (best-effort); continuing"; return 0
  fi
  cp -f target/vuln-arm64 exploit/

  # Choose a runner: native on aarch64, else qemu.
  local runner=""
  if [ "$HOST" != aarch64 ]; then
    qemu="$(qemu_arm)"
    if [ -z "$qemu" ]; then
      echo "WARN  no qemu-aarch64 to run the AArch64 target (best-effort); skipping run"
      return 0
    fi
    runner="$qemu -L /usr/aarch64-linux-gnu"
  fi

  echo "==== AArch64 smoke (target runs?) ===="
  if echo | ( cd exploit && timeout 30 $runner ./vuln-arm64 ) 2>&1 | grep -q 'ch08 rop lab'; then
    echo "PASS  AArch64 target runs"
    [ "$hard" = 1 ] && ran_hard=1
  else
    if [ "$hard" = 1 ]; then echo "FAIL  AArch64 target did not run"; rc=1; else
      echo "WARN  AArch64 smoke failed (best-effort)"; fi
  fi

  echo "==== AArch64 ret2libc (solve_arm64.py, best-effort) ===="
  if run_script exploit solve_arm64.py; then
    echo "PASS  AArch64 ret2libc"
  else
    echo "WARN  AArch64 ret2libc did not confirm a shell (best-effort; verify on"
    echo "      real aarch64 hardware — see exploit/solve_arm64.py header)"
  fi
}

case "$LANE" in
  x86_64)
    if x86_buildable; then do_x86; else echo "no x86-64 toolchain on this host"; fi
    ;;
  arm64)  do_arm ;;
  *)
    if x86_buildable; then do_x86; fi
    do_arm
    ;;
esac

if [ "$ran_hard" = 0 ]; then
  echo "ERROR: no hard gate ran on this host — nothing was validated."
  exit 1
fi

echo
if [ "$rc" -eq 0 ]; then echo "ch08-rop: all hard gates passed."; else echo "ch08-rop: FAILURES above."; fi
exit "$rc"
