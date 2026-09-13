#!/usr/bin/env python3
"""Demonstrate how a size-calculation integer overflow becomes a
heap overflow — the pattern behind hundreds of real CVEs."""

# Simulate the bug: allocation size = count * elem_size
elem_size = 32                                    # per record
attacker_count = 0x0800_0000                      # ~134 million records
allocation_size_wanted = attacker_count * elem_size

# On a 32-bit size_t (or C's `size_t` on some ABIs), this wraps:
UINT32 = 1 << 32
wrapped = allocation_size_wanted % UINT32

print("=== integer overflow -> heap overflow (Chapter 7) ===")
print(f"element size:            {elem_size} bytes")
print(f"attacker-supplied count: {attacker_count:#x}  ({attacker_count:,})")
print(f"logical size wanted:     {allocation_size_wanted:#x}  ({allocation_size_wanted:,} bytes)")
print(f"32-bit size_t sees:      {wrapped:#x}  ({wrapped} bytes)  <-- WRAPPED")

print(f"\nallocator gets {wrapped} bytes")
print(f"then the loop writes {attacker_count} * {elem_size} = "
      f"{allocation_size_wanted:,} bytes past the wrap point")
print(f"overflow into next chunk: {allocation_size_wanted - wrapped:,} bytes")

print("\n--- the fix (C23 checked arithmetic) ---")
print("#include <stdckdint.h>")
print("size_t total;")
print("if (ckd_mul(&total, count, sizeof *r)) return -EOVERFLOW;")
print("records = calloc(1, total);")

print("\nUBSan under -fsanitize=undefined catches this at runtime:")
print("size.c:14:26: runtime error: unsigned integer overflow: "
      "134217728 * 32 cannot be represented in type 'unsigned int'")
