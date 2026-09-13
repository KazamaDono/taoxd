#!/usr/bin/env bash
# Decide whether ret2csu is viable for a target.
# glibc >= 2.34 (Ubuntu 24.04 ships 2.39) drops __libc_csu_init.
set -euo pipefail
bin="${1:?usage: csu_probe.sh <binary>}"

if nm "$bin" 2>/dev/null | grep -q '__libc_csu_init' \
   || objdump -d "$bin" 2>/dev/null | grep -q '<__libc_csu_init>'; then
    echo "[+] __libc_csu_init present -> classic ret2csu is viable"
else
    echo "[-] no __libc_csu_init (glibc >= 2.34 build)."
    echo "    Use ret2dlresolve, a libc setcontext gadget, or libc-internal"
    echo "    arg-setting gadgets located after a leak."
fi
