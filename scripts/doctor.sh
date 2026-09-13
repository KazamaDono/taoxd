#!/usr/bin/env bash
# doctor.sh — verify the lab toolchain is present and print versions.
set -uo pipefail
ok=0; miss=0
check() {
  local name="$1"; shift
  if "$@" >/dev/null 2>&1; then
    printf '  \033[32mok\033[0m   %-16s %s\n' "$name" "$("$@" 2>&1 | head -1)"
    ok=$((ok+1))
  else
    printf '  \033[31mMISS\033[0m %-16s\n' "$name"; miss=$((miss+1))
  fi
}
echo "Toolchain check:"
check gcc            gcc --version
check g++            g++ --version
check clang          clang --version
check aarch64-gcc    aarch64-linux-gnu-gcc --version
check qemu-aarch64   qemu-aarch64-static --version
check gdb            gdb --version
check nasm           nasm --version
check python3        python3 --version
check pwntools       python3 -c "import pwn; print('pwntools', pwn.__version__)"
check ROPgadget      ROPgadget --version
check ropper         ropper --version
check one_gadget     one_gadget --version
check patchelf       patchelf --version
echo
echo "  present: $ok   missing: $miss"
[[ $miss -eq 0 ]] && echo "  lab ready." || echo "  some tools missing — rebuild the image (./scripts/lab.sh --build)."
