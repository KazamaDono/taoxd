#!/usr/bin/env python3
"""Watch GOT[puts] move from the PLT resolver stub to libc's puts.
Target: ch03-arch/gotplt/greeter, Ubuntu 24.04 / glibc 2.39."""
from pwn import *

elf = ELF('./greeter', checksec=False)
context.arch = elf.arch       # 'amd64' on x86-64, 'aarch64' on arm64 — CI tests both

io, g = gdb.debug('./greeter', api=True,
                  gdbscript='set disable-randomization on')

# Stop at main; compute the PIE load base from a known symbol.
g.Breakpoint('main', temporary=True)          # (1)
g.continue_and_wait()
base = int(g.parse_and_eval('(long)&main')) - elf.sym['main']   # (2)
got_puts = base + elf.got['puts']             # (3)

def deref(addr):
    return int(g.parse_and_eval(f'*(unsigned long *){addr}')) & (2**64 - 1)

before = deref(got_puts)
log.info('GOT[puts] @ %#x  before 1st call = %#x  (PLT resolver path)',
         got_puts, before)

g.Breakpoint('puts', temporary=True)          # (4)
g.continue_and_wait()                         # fires inside libc puts
after = deref(got_puts)
log.info('GOT[puts] @ %#x  after  1st call = %#x  (libc puts)',
         got_puts, after)

assert before != after, 'expected lazy binding to patch the slot'
log.success('lazy binding confirmed: %#x -> %#x', before, after)
g.execute('detach'); io.close()
