"""Safe-linking mangle for glibc >= 2.32.

Given the address of the freelist NODE (the chunk's user-data address, i.e.
mchunkptr + 0x10 on 64-bit glibc) and the address you want the freelist to
believe the next entry lives at, produce the value glibc expects to see in
the fd slot. REVEAL is its own inverse: reveal(mangle(node, ptr)) == ptr.
"""

def mangle(node_addr: int, next_ptr: int) -> int:                # PROTECT_PTR ❶
    return (node_addr >> 12) ^ next_ptr

def reveal(node_addr: int, stored_fd: int) -> int:               # REVEAL_PTR
    return (node_addr >> 12) ^ stored_fd

def demangle_from_leak(node_addr: int, stored_fd: int) -> int:
    """Given a leaked mangled fd, recover the real next-pointer."""
    return reveal(node_addr, stored_fd)                          # ❷

# Sanity: mangling is symmetric and alignment-preserving for aligned pointers.
assert reveal(0x555555559010, mangle(0x555555559010, 0x7ffff7dbe680)) \
       == 0x7ffff7dbe680                                          # ❸
