#!/usr/bin/env bash
# Boots Linux 6.8 in QEMU with a BusyBox initramfs that auto-insmods vuln.ko.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
KERNEL="$here/artifacts/bzImage"                  # [1] pinned 6.8 build
INITRD="$here/artifacts/initramfs.cpio.gz"
APPEND="console=ttyS0 panic=-1 oops=panic nokaslr"  # [2] deterministic panic

QEMU_ARGS=(
    -kernel "$KERNEL" -initrd "$INITRD" -append "$APPEND"
    -m 512M -smp 2 -cpu qemu64,+smep,+smap         # [3] SMEP/SMAP on
    -nographic -no-reboot                          # [4] die, don't loop
    -s                                             # [5] gdb stub on :1234
)
[[ -n "${DEBUG:-}" ]] && QEMU_ARGS+=(-S)           # [6] freeze at boot

exec qemu-system-x86_64 "${QEMU_ARGS[@]}"
