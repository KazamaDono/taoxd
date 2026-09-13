#!/usr/bin/env python3
"""Recover the ch04 crackme key and verify it against the binary."""
import sys
from pwn import *

context.log_level = 'info'

# Lifted from .rodata via Ghidra (double-click `target` in the decompiler).
TARGET = bytes([0x22, 0x4a, 0x60, 0x0e, 0x22, 0x0e, 0x5f, 0x50,
                0x55, 0x69, 0x3c, 0x2a, 0x13, 0x12, 0x0f, 0x08])
IV = 0x2a                                              # ❶ the feedback seed

def recover(target):
    key = bytearray(len(target))
    prev = IV
    for i, t in enumerate(target):
        key[i] = t ^ ((0x5a + i) & 0xff) ^ prev        # ❷ invert the transform
        prev = t                                       # ❸ feedback uses the output
    return bytes(key)

def main():
    binary = sys.argv[1] if len(sys.argv) > 1 else './crackme-x86_64'
    key = recover(TARGET)
    log.success('recovered key: %r', key)              # ❹ -> b'R3v3rs1ng_1s_fun'

    io = process([binary, key.decode()])
    out = io.recvall(timeout=5)
    io.close()
    assert b'Correct' in out, out                      # ❺ CI asserts the win
    log.success('%s accepted the key', binary)

if __name__ == '__main__':
    main()
