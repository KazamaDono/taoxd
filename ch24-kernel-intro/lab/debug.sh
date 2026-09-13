#!/usr/bin/env bash
# ch24-kernel-intro/lab/debug.sh — attach gdb + pwndbg to the QEMU stub
# started by `DEBUG=1 ./lab/run.sh`. Loads vmlinux unstripped so kernel
# symbols resolve, and drops you at a gdb prompt from which you can
# `add-symbol-file ./vuln/vuln.ko <mod_base>` and break on `vuln_write`.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VMLINUX="${ROOT}/artifacts/vmlinux"
STUB="${STUB:-:1234}"

if [[ ! -f "$VMLINUX" ]]; then
    echo "no artifacts/vmlinux — build the kernel first (see run.sh)" >&2
    exit 1
fi

# Load pwndbg (installed system-wide in the lab image; see the repo README)
GDBINIT="$HOME/.gdbinit"
[[ -f /opt/pwndbg/gdbinit.py ]] && ! grep -q pwndbg "$GDBINIT" 2>/dev/null && \
    echo "source /opt/pwndbg/gdbinit.py" >> "$GDBINIT"

exec gdb -q "$VMLINUX" \
    -ex "target remote $STUB" \
    -ex "set print pretty on" \
    -ex "set pagination off"
