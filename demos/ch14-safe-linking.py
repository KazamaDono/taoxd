#!/usr/bin/env python3
"""Demonstrate glibc 2.32+ safe-linking pointer mangling.

Safe-linking XORs the tcache/fastbin `fd` pointer with the *position*
of the chunk (shifted right by 12). This means a naive tcache-poisoning
attempt that writes a raw pointer produces garbage on dequeue — an
attacker needs a heap-address leak to compute the correct mangled value.
"""
import sys

def mangle(pos: int, ptr: int) -> int:
    """Encode a next-pointer for glibc 2.32+ safe-linking."""
    return (pos >> 12) ^ ptr

def demangle(pos: int, mangled: int) -> int:
    """Inverse: recover the real pointer from the mangled value."""
    return (pos >> 12) ^ mangled

# Simulated heap chunk on a 64-bit process:
CHUNK_POS  = 0x555555559420   # where the freed chunk lives
NEXT_PTR   = 0x5555555593d0   # the real next pointer glibc writes there

print("=== glibc 2.32+ tcache safe-linking demo ===")
print(f"chunk position (leaked):  {CHUNK_POS:#018x}")
print(f"real next-pointer:        {NEXT_PTR:#018x}")

encoded = mangle(CHUNK_POS, NEXT_PTR)
print(f"\nmangled value at fd:      {encoded:#018x}")
print(f"  = (pos >> 12) XOR next")
print(f"  = {CHUNK_POS >> 12:#018x} XOR {NEXT_PTR:#018x}")

roundtrip = demangle(CHUNK_POS, encoded)
print(f"\ndemangled (allocator sees): {roundtrip:#018x}")
assert roundtrip == NEXT_PTR, "round-trip failed"
print("round-trip OK\n")

# The attacker's problem: write a raw pointer with no leak
ATTACKER_TARGET = 0x7ffff7e19b70   # e.g. __free_hook (glibc<=2.33) or _IO vtable
print("--- WITHOUT a heap-address leak ---")
print(f"attacker writes raw pointer to fd: {ATTACKER_TARGET:#018x}")
print(f"allocator XORs on dequeue with:    {CHUNK_POS >> 12:#018x}")
garbage = mangle(CHUNK_POS, ATTACKER_TARGET)
print(f"allocator returns:                 {garbage:#018x}  <-- garbage; SIGSEGV")

print("\n--- WITH a heap-address leak ---")
print(f"attacker knows chunk pos, writes:  {mangle(CHUNK_POS, ATTACKER_TARGET):#018x}")
print(f"allocator dequeues and returns:    {ATTACKER_TARGET:#018x}  <-- controlled!")
