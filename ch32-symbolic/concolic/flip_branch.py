#!/usr/bin/env python3
"""Concolic driver: pin symbolic stdin to a concrete seed, walk with the
Tracer technique, drop the seed-equality at a chosen state, and re-solve
for a nearby input that diverges.

Usage:  flip_branch.py [BINARY] [TARGET_HEX_ADDR]
Both arguments are optional; TARGET_HEX_ADDR defaults to the do_copy
symbol of the parser lab target.
"""
import sys
import angr, claripy
from angr.exploration_techniques import Tracer

BIN = sys.argv[1] if len(sys.argv) > 1 else './parser-x86_64'

p = angr.Project(BIN, auto_load_libs=False)
seed = b'A' * 32
sym_stdin = claripy.BVS('stdin', 8 * len(seed))

state = p.factory.entry_state(
    stdin=angr.SimFileStream(name='stdin', content=sym_stdin, has_end=True))
state.solver.add(sym_stdin == seed)                       # ❶ pin to concrete seed

simgr = p.factory.simgr(state)
simgr.use_technique(Tracer(trace=None, crash_addr=None))  # ❷ dynamic trace

# Walk one basic block at a time; at each branch, record and optionally flip.
if len(sys.argv) > 2:
    target_pc = int(sys.argv[2], 0)                       # ❸ addr we want to reach
else:
    target_pc = p.loader.find_symbol('do_copy').rebased_addr
while simgr.active:
    simgr.step()
    for s in simgr.active:
        if s.addr == target_pc:
            s.solver.add(sym_stdin != seed)               # ❹ don't return the seed
            new = s.solver.eval(sym_stdin, cast_to=bytes)
            print('diverging input:', new.hex())
            raise SystemExit
