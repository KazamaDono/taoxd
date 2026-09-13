#!/usr/bin/env python3
"""Chapter 10 — the leak-and-rebase arithmetic every modern exploit runs.
One leaked address plus one on-disk offset lets you compute every other
symbol's runtime address in that library."""

# Numbers a real exploit would harvest:
leaked_puts_runtime = 0x00007f4a2ce80c30    # from a stack leak

# From `nm /lib/x86_64-linux-gnu/libc.so.6 | grep -E ' T (puts|system|execve)$'`
# on the pinned Ubuntu 24.04 glibc 2.39:
sym_offset = {
    "puts":         0x0000000000080c30,
    "system":       0x0000000000050d70,
    "execve":       0x00000000000e8280,
    "__libc_start_main": 0x0000000000029d90,
    "environ":      0x000000000021a4a0,   # global — useful for stack leaks
}

# Rebase: base = leaked - offset_on_disk
base = leaked_puts_runtime - sym_offset["puts"]

print("=== leak-and-rebase: turn one address into a symbol map ===")
print(f"leaked libc puts address:  {leaked_puts_runtime:#018x}")
print(f"puts offset on disk:       {sym_offset['puts']:#018x}")
print(f"libc load base at runtime: {base:#018x}")
print(f"  (page-aligned? {'yes' if base & 0xfff == 0 else 'NO'}  — sanity check)")
print()
print("every other symbol in this libc is one addition away:")
for name, off in sorted(sym_offset.items(), key=lambda x: x[1]):
    print(f"  {name:24s}  = base + {off:#08x}  = {base + off:#018x}")

print("\nnow the ret2libc chain writes itself:")
print(f"  pop rdi ; ret        # gadget, from libc")
print(f"  {base + sym_offset['execve']:#018x}   # address of '/bin/sh' (from libc.search)")
print(f"  {base + sym_offset['system']:#018x}   # -> system('/bin/sh')")
