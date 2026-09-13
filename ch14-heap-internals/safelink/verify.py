#!/usr/bin/env python3
# Reads safelink stdout on stdin, asserts the four invariants from
# Listing 14-3 of the book. Exits 0 on success, 1 on failure.
import sys, re

lines = sys.stdin.read().splitlines()

def find(pat):
    for L in lines:
        m = re.search(pat, L)
        if m: return m
    print("FAIL missing pattern:", pat); sys.exit(1)

m = find(r'^c=0x[0-9a-f]+\s+raw c->next=0x[0-9a-f]+\s+reveal=(0x[0-9a-f]+)\s+\(expect b=(0x[0-9a-f]+)\)$')
assert int(m.group(1),16) == int(m.group(2),16), "c reveal != b"
print("OK c reveal == b")

m = find(r'^b=0x[0-9a-f]+\s+raw b->next=0x[0-9a-f]+\s+reveal=(0x[0-9a-f]+)\s+\(expect a=(0x[0-9a-f]+)\)$')
assert int(m.group(1),16) == int(m.group(2),16), "b reveal != a"
print("OK b reveal == a")

find(r'^a=0x[0-9a-f]+\s+raw a->next=0x[0-9a-f]+\s+reveal=(\(nil\)|0x0+)\s+\(expect NULL\)$')
print("OK a reveal == NULL")

m = find(r'^heap base leak from tail: (0x[0-9a-f]+) \(== a>>12: (0x[0-9a-f]+)\)$')
assert int(m.group(1),16) == int(m.group(2),16), "tail-leak != a>>12"
print("OK tail-leak == a>>12 (safe-linking tail sentinel)")

print("PASS ch14-safelink")
