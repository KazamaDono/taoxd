#!/usr/bin/env bash
# lab.sh — build or enter the pinned Ubuntu 24.04 lab container.
#   ./scripts/lab.sh --build        build (or rebuild) the image
#   ./scripts/lab.sh                 open an interactive lab shell
#   ./scripts/lab.sh make test-all   run a command inside the lab
set -euo pipefail

IMAGE="modern-exploit-dev-lab:24.04"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "${1:-}" == "--build" ]]; then
  docker build -t "$IMAGE" -f "$REPO_ROOT/lab/Dockerfile" "$REPO_ROOT"
  exit 0
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "[*] Lab image not found; building it once…"
  docker build -t "$IMAGE" -f "$REPO_ROOT/lab/Dockerfile" "$REPO_ROOT"
fi

# SYS_PTRACE + unconfined seccomp so gdb and `setarch -R` (ASLR off) work
# *inside the container only*. The host is never modified.
# PWNLIB_NOTERM=1 disables pwntools' interactive "terminal mode". With a real
# TTY (we pass -it) pwntools otherwise takes over the terminal and the
# format-string / brute solves hang waiting on cursor-position replies. Turning
# it off changes nothing about how the exploits run (CI runs without a TTY
# anyway) — it just keeps the labs from stalling in an interactive terminal.
exec docker run --rm -it \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  -e PWNLIB_NOTERM=1 \
  -v "$REPO_ROOT":/work \
  -w /work \
  "$IMAGE" "${@:-bash}"
