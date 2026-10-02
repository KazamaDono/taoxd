#!/usr/bin/env python3
"""Watch GOT[puts] move from the PLT resolver stub to libc's puts.
Target: ch03-arch/gotplt/greeter, Ubuntu 24.04 / glibc 2.39.

Driven with gdb in *batch* mode (gdb -batch -ex ...) rather than pwntools'
interactive gdb API. The API path (gdb.debug(..., api=True)) has to spawn gdb
in a terminal, which does not exist on a headless CI runner — there it failed
with "cannot unpack non-iterable process object". Batch mode needs no terminal
and is deterministic.
"""
import re
import subprocess
from pwn import ELF, context, log

elf = ELF('./greeter', checksec=False)
context.arch = elf.arch        # 'amd64' on x86-64, 'aarch64' on arm64
main_off = elf.sym['main']     # static (PIE) offsets; add the runtime base
got_off  = elf.got['puts']

# Break at main (before any puts call), compute the PIE base from &main, read
# GOT[puts] (still the lazy PLT resolver stub), then break in libc puts on the
# first call (by which point the resolver has patched the slot) and read again.
gdb_cmds = [
    'set pagination off',
    'set disable-randomization on',
    'break main', 'run',
    f'set $base = (unsigned long)&main - {main_off}',
    f'set $got  = $base + {got_off}',
    'printf "BASE %#lx\\n", $base',
    'printf "BEFORE %#lx\\n", *(unsigned long *)$got',
    'tbreak puts', 'continue',
    'printf "AFTER %#lx\\n", *(unsigned long *)$got',
    'kill', 'quit',
]
cmd = ['gdb', '-q', '-nx', '-batch', './greeter']
for c in gdb_cmds:
    cmd += ['-ex', c]

out = subprocess.run(cmd, capture_output=True, text=True, timeout=120).stdout
log.info('gdb batch output:\n%s', out.strip())

def grab(tag):
    m = re.search(rf'{tag} (0x[0-9a-fA-F]+)', out)
    if not m:
        log.error('could not find %s in gdb output (see above)', tag)
    return int(m.group(1), 16)

base   = grab('BASE')
before = grab('BEFORE')
after  = grab('AFTER')
got_puts = base + got_off

log.info('GOT[puts] @ %#x  before 1st call = %#x  (PLT resolver path)', got_puts, before)
log.info('GOT[puts] @ %#x  after  1st call = %#x  (libc puts)', got_puts, after)

assert before != after, 'expected lazy binding to patch the slot'
log.success('lazy binding confirmed: %#x -> %#x', before, after)
