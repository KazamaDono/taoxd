#!/usr/bin/env python3
"""ch5: De Bruijn cyclic pattern — the fastest way to find a stack
overflow's offset. Generate a unique pattern, feed it as input, read the
faulting bytes off the register, look them up. No guessing, no counting."""

def cyclic(n, alphabet=b"abcdefghijklmnopqrstuvwxyz"):
    """De Bruijn B(len(alphabet), 4) sequence, truncated to n bytes.
    Every 4-byte window is unique — so any 4 leaked bytes tell you their
    exact position."""
    k = 4
    a = [0] * (k * len(alphabet))
    seq = []
    def db(t, p):
        if t > k:
            if k % p == 0: seq.extend(a[1:p + 1])
        else:
            a[t] = a[t - p]; db(t + 1, p)
            for j in range(a[t - p] + 1, len(alphabet)):
                a[t] = j; db(t + 1, t)
    db(1, 1)
    return bytes(alphabet[i] for i in seq)[:n]

def cyclic_find(needle: bytes, pattern: bytes) -> int:
    """Where does `needle` appear in the pattern? -1 if not present."""
    return pattern.find(needle)

# Demo — build a pattern, "crash" at a known offset, look it up
pat = cyclic(300)
print("=== De Bruijn cyclic pattern for stack-overflow offset finding ===")
print(f"pattern length:   {len(pat)} bytes")
print(f"first 64 bytes:   {pat[:64].decode()}")
print()

# Simulate: attacker sends the pattern, gets a segfault, gdb shows rip
# was overwritten with these 8 bytes.
crash_bytes = b"kaaalaaa"   # 8 bytes from the pattern that landed in rip
offset = cyclic_find(crash_bytes, pat)
print(f"gdb reports:      rip = {crash_bytes!r}")
print(f"cyclic_find:      offset {offset}")
print(f"                  -> saved return address is at padding + {offset}")
print()

# Sanity: prove offset is correct by extracting from the pattern
print(f"pattern[{offset}:{offset+8}] = {pat[offset:offset+8]!r}   <-- matches, offset confirmed")
print()

# One more — the second crash the attacker triggers, different 4 bytes:
crash2 = b"faab"
off2 = cyclic_find(crash2, pat)
print(f"second crash:     rip low 4 = {crash2!r}  ->  offset {off2}  (canary lives here)")
