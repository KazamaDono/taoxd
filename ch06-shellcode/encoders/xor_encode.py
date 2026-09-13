#!/usr/bin/env python3
"""Single-byte XOR-encode shellcode so the result avoids forbidden bytes.
Emits the encoded payload; pair with decoder_x64.S at runtime."""
import sys

def encode(payload: bytes, bad: set[int]) -> tuple[int, bytes]:
    for key in range(1, 256):                       # ❶ try every single-byte key
        if key in bad:
            continue
        enc = bytes(b ^ key for b in payload)
        if not (set(enc) & bad):                    # ❷ no encoded byte is forbidden
            return key, enc
    raise ValueError("no single-byte key works; use a multi-byte schema")

if __name__ == "__main__":
    blob = open(sys.argv[1], "rb").read()
    bad = {0x00, 0x0a, 0x0d}                         # ❸ NUL, LF, CR as an example
    key, enc = encode(blob, bad)
    sys.stderr.write(f"[*] key=0x{key:02x}  len={len(enc)}\n")
    sys.stdout.buffer.write(enc)
