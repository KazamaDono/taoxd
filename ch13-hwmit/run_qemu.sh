#!/usr/bin/env bash
# Boot a QEMU AArch64 user-mode session with MTE enabled and run the ch13
# demo suite. For a full-system boot (needed if you want to test kernel-mode
# MTE), pass --system and we launch qemu-system-aarch64 -cpu max,mte=on with
# an Ubuntu 24.04 aarch64 cloud image (path in $UBUNTU_ARM64_IMG).
set -euo pipefail
cd "$(dirname "$0")"

mode="user"
[[ "${1-}" == "--system" ]] && mode="system"

if [[ "$mode" == "user" ]]; then
    echo "== ch13 demo suite (qemu-user, -cpu max) =="
    make -s -C pac test || true
    make -s -C mte test || true
    make -s -C cet test || true
    exit 0
fi

: "${UBUNTU_ARM64_IMG:?set UBUNTU_ARM64_IMG to an Ubuntu 24.04 aarch64 qcow2}"
exec qemu-system-aarch64 \
    -M virt -cpu max,mte=on -m 4G -smp 4 -nographic \
    -bios /usr/share/AAVMF/AAVMF_CODE.fd \
    -drive if=virtio,format=qcow2,file="$UBUNTU_ARM64_IMG"
