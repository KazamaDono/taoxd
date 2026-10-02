#!/usr/bin/env bash
# doctor.sh — verify the lab toolchain is present and matches the pinned
# versions the book was written against (Figure 2-1 / lab/requirements.txt).
#
# Two kinds of problem are reported distinctly:
#   MISS   — the tool is absent or not usable (hard failure; rebuild needed).
#   DRIFT  — the tool is present but its version differs from the pin. The lab
#            still runs, but exploits authored against the pinned versions may
#            behave differently. This is the toolchain drift behind issue #1,
#            so it is now surfaced instead of being silently tolerated.
set -uo pipefail

ok=0; miss=0; drift=0

paint() { printf '\033[%sm%s\033[0m' "$1" "$2"; }

# check <name> <expected|-> <probe-cmd...>
#   The probe must exit 0 and print a version string on stdout when the tool is
#   present. <expected> is a substring that must appear in that string; use "-"
#   for tools we deliberately do not version-lock (the apt base toolchain).
#   Note: we capture the probe's full output in one shot (no pipe) so a tool
#   that works can never be misreported as MISS because of a SIGPIPE from
#   `head` — the false-negative class that made pwntools read as MISS before.
check() {
  local name="$1" expected="$2"; shift 2
  local full first
  if ! full="$("$@" 2>/dev/null)" || [[ -z "$full" ]]; then
    printf '  %s %-16s\n' "$(paint 31 MISS)" "$name"; miss=$((miss+1)); return
  fi
  first="${full%%$'\n'*}"
  if [[ "$expected" != "-" && "$first" != *"$expected"* ]]; then
    printf '  %s %-16s %s  %s\n' \
      "$(paint 33 DRIFT)" "$name" "$first" "$(paint 33 "(expected $expected)")"
    drift=$((drift+1)); return
  fi
  printf '  %s   %-16s %s\n' "$(paint 32 ok)" "$name" "$first"; ok=$((ok+1))
}

# pwntools no longer exposes `pwn.__version__`; import it to prove it is usable,
# then read the installed version from package metadata (stable across releases).
pwntools_probe() {
  python3 - <<'PY'
import pwn  # noqa: F401  (import proves the toolkit is importable/usable)
import importlib.metadata as m
print("pwntools", m.version("pwntools"))
PY
}

echo "Toolchain check:"
#     name            expected pin   probe command
check gcc            -              gcc --version
check g++            -              g++ --version
check clang          -              clang --version
check aarch64-gcc    -              aarch64-linux-gnu-gcc --version
check qemu-aarch64   -              qemu-aarch64-static --version
check gdb            -              gdb --version
check nasm           -              nasm --version
check python3        -              python3 --version
check pwntools       4.12.0         pwntools_probe
check ROPgadget      7.4            ROPgadget --version
check ropper         1.13.8         ropper --version
check one_gadget     1.9.0          one_gadget --version
check patchelf       -              patchelf --version
echo
echo "  present: $ok   missing: $miss   drift: $drift"
if [[ $miss -gt 0 ]]; then
  echo "  some tools missing — rebuild the image (./scripts/lab.sh --build)."
  exit 1
elif [[ $drift -gt 0 ]]; then
  echo "  toolchain drift detected — versions differ from the book's pins."
  echo "  rebuild the pinned image to match: ./scripts/lab.sh --build"
  exit 0
else
  echo "  lab ready."
  exit 0
fi
