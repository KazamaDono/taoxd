#!/usr/bin/env bash
# ch12-cfi/kcfi/kcfi-probe.sh
#
# Scan a Linux 6.8 kernel module (or vmlinux slice) built with
# CONFIG_CFI_CLANG=y for the mov/add/jne kCFI prologue-check pattern shown
# in Listing 12.4 ({{ref:lst-kcfi}}).
#
# Usage:
#   ./kcfi-probe.sh <path-to-.ko-or-elf>
#   ./kcfi-probe.sh --self-test        # generate a synthetic ELF and probe it
#
# On a real kCFI kernel module the output looks like:
#
#   [+] scanning drivers/net/e1000/e1000.ko
#   [+] disassembling with objdump -d --no-show-raw-insn
#   [+] found 137 kCFI check sites (mov imm32 / add [reg-4] / jne)
#   [+] example:
#       ffffffffc02341a0: mov    $0x12345678,%eax
#       ffffffffc02341a5: add    -0x4(%r11),%eax
#       ffffffffc02341a9: jne    ffffffffc02341b2
#       ffffffffc02341ab: call   *%r11
#
# Exit status: 0 if at least one kCFI site was found (or self-test passed),
# nonzero otherwise.  CI's --self-test path does not need a real kernel.

set -euo pipefail

self_test() {
    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN

    # Emit a tiny x86-64 ELF that contains a synthesized kCFI check
    # sequence for the pattern-matcher to find.  We assemble it with
    # `as` / `ld`; both are in binutils and always present.
    cat > "$tmp/probe.s" <<'ASM'
        .text
        .globl _start
_start:
        mov     $0x12345678, %eax
        add     -0x4(%r11), %eax
        jne     .Ltrap
        callq   *%r11
.Ltrap:
        ud2
        ret
ASM
    as --64 -o "$tmp/probe.o" "$tmp/probe.s"
    ld -o "$tmp/probe.elf" "$tmp/probe.o" 2>/dev/null || true
    # ld may complain about the missing _start alignment; the .o is enough
    # for objdump to disassemble.
    probe "$tmp/probe.o"
}

probe() {
    local target="$1"
    if [[ ! -r "$target" ]]; then
        echo "error: cannot read $target" >&2
        return 2
    fi
    echo "[+] scanning $target"
    echo "[+] disassembling with objdump -d --no-show-raw-insn"

    local dis
    dis="$(objdump -d --no-show-raw-insn "$target" 2>/dev/null || true)"
    if [[ -z "$dis" ]]; then
        echo "error: objdump produced no output for $target" >&2
        return 3
    fi

    # The kCFI check on x86-64 is a three-instruction preamble on every
    # indirect call site:
    #     mov  $imm32, %eax          ; expected hash
    #     add  -0x4(%r<n>), %eax     ; add stored hash at target-4
    #     jne  .Ltrap                ; if nonzero, trap
    # We tolerate any GPR in the add operand (Clang picks the target reg).
    local hits
    hits="$(printf '%s\n' "$dis" | awk '
        /mov[[:space:]]+\$0x[0-9a-fA-F]+,%eax/                 { m=NR; last_mov=$0; next }
        /add[[:space:]]+-0x4\(%r[0-9a-z]+\),%eax/ && NR==m+1   { a=NR; last_add=$0; next }
        /jne[[:space:]]/                            && NR==a+1 { print last_mov; print $0; print "---"; count++ }
        END { print "COUNT="count+0 }
    ')"

    local count
    count="$(printf '%s\n' "$hits" | awk -F= '/^COUNT=/{print $2}')"
    count="${count:-0}"

    echo "[+] found $count kCFI check sites (mov imm32 / add [reg-4] / jne)"
    if [[ "$count" -gt 0 ]]; then
        echo "[+] example:"
        printf '%s\n' "$hits" | awk '/---/{c++; if(c==1) exit} c==0 && NF' | sed 's/^/    /'
        return 0
    fi
    echo "[-] no kCFI prologue-check pattern found -- module may not be" \
         "built with CONFIG_CFI_CLANG, or it is stripped."
    return 1
}

main() {
    if [[ $# -lt 1 ]]; then
        echo "usage: $0 <module.ko|vmlinux> | --self-test" >&2
        exit 64
    fi
    case "$1" in
        --self-test) self_test ;;
        -h|--help)   echo "usage: $0 <module.ko|vmlinux> | --self-test"; exit 0 ;;
        *)           probe "$1" ;;
    esac
}

main "$@"
