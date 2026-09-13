#!/usr/bin/env python3
"""ch11: simulate the byte-at-a-time stack-canary brute against a
fork-server. On every wrong byte the child crashes and the parent
forks a fresh one with the SAME canary; on every right byte, no
crash. 256 tries per byte, 8 bytes -> 2048 requests total to leak
an 8-byte canary. Simulated deterministically here so you can watch
the algorithm play out."""
import random

# ONE canary the "target process" carries — glibc always keeps the low
# byte at \x00 so a strcpy-style overflow can't leak the whole thing by
# accident.
target_canary = bytes.fromhex("00c3b8a742d907e1")   # low byte = 0x00
assert target_canary[0] == 0, "glibc canary must have NUL low byte"

def probe(guess: bytes, canary: bytes) -> bool:
    """Simulate one connection to the fork-server: attacker's guess
    matches the corresponding prefix of the real canary -> no crash.
    Otherwise -> child crashes, parent forks a fresh one."""
    return canary.startswith(guess)

leaked = b""
tries = 0
print("=== canary byte-by-byte brute against a fork-server ===")
print(f"target canary (unknown to us): {target_canary.hex()}")
print()
for pos in range(8):
    for b in range(256):
        tries += 1
        guess = leaked + bytes([b])
        if probe(guess, target_canary):
            leaked = guess
            print(f"  byte {pos}: 0x{b:02x}  (found in {b+1} tries)")
            break

print()
print(f"total probes:  {tries}")
print(f"leaked canary: {leaked.hex()}")
print(f"matches?       {leaked == target_canary}")
print()
print("This is why fork-servers are so exploitable: 2048 probes at a few")
print("ms each is a fifteen-second job; no ASLR bypass, no info leak, no")
print("mitigation catches it. Real targets that do this: legacy Apache")
print("prefork, some SSH configs, hand-rolled network services.")
