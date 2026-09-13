#!/usr/bin/env python3
"""Solve the xorcheck crackme with angr. angr 9.2 on Python 3.12."""
import sys, angr, claripy

BIN = sys.argv[1] if len(sys.argv) > 1 else './xorcheck-x86_64'
KEYLEN = 8

p = angr.Project(BIN, auto_load_libs=False)               # ❶

# 8 symbolic bytes, printable-ASCII constrained.
key_bytes = [claripy.BVS(f'k{i}', 8) for i in range(KEYLEN)]
argv = [BIN, claripy.Concat(*key_bytes)]                  # ❷
state = p.factory.entry_state(args=argv)
for b in key_bytes:
    state.solver.add(b >= 0x20, b < 0x7f)                 # ❸

find  = p.loader.find_symbol('win').rebased_addr          # ❹
avoid = p.loader.find_symbol('lose').rebased_addr

simgr = p.factory.simgr(state)
simgr.explore(find=find, avoid=[avoid])                   # ❺

if not simgr.found:
    sys.exit('no state reached win')

sol = simgr.found[0]
key = b''.join(sol.solver.eval(b, cast_to=bytes) for b in key_bytes)  # ❻
print('key:', key.decode())
