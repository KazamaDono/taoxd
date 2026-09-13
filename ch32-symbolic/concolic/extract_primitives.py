#!/usr/bin/env python3
"""Iterate simgr.unconstrained on a crashing binary and classify each
state as pc-control / write-what-where / neither.  Solves Exercise 4.

Usage:  extract_primitives.py [BINARY]
"""
import sys, angr, claripy

BIN = sys.argv[1] if len(sys.argv) > 1 else './parser-x86_64'

p = angr.Project(BIN, auto_load_libs=False)
INBUF = 64
inp = claripy.BVS('inp', INBUF * 8)
state = p.factory.entry_state(
    stdin=angr.SimFileStream(name='stdin', content=inp, has_end=True))

simgr = p.factory.simgr(state, save_unconstrained=True)
# Explore until we accumulate some unconstrained states or run out.
simgr.run(n=200)

if not simgr.unconstrained:
    print('[!] no unconstrained states surfaced (try more steps or a richer seed)')
    sys.exit(0)

for i, s in enumerate(simgr.unconstrained):
    ip_sym = s.regs.ip
    pc_ctrl = s.solver.symbolic(ip_sym)
    print(f'[{i}] pc-control: {pc_ctrl}')
    if pc_ctrl and s.solver.satisfiable(extra_constraints=[ip_sym == 0xdeadbeef]):
        poc = s.solver.eval(s.posix.stdin.load(0, INBUF), cast_to=bytes)
        print(f'    -> input driving PC to 0xdeadbeef: {poc.hex()}')
    # Write-what-where check: last stored expression, if the engine tracked one.
    try:
        last_action = next((a for a in reversed(list(s.history.actions))
                            if a.type == 'mem' and a.action == 'write'), None)
        if last_action is not None:
            addr_sym = s.solver.symbolic(last_action.addr.ast)
            data_sym = s.solver.symbolic(last_action.data.ast)
            print(f'    last store: addr_sym={addr_sym} data_sym={data_sym}')
            if addr_sym and data_sym:
                print('    -> classified as write-what-where')
    except Exception as e:
        print(f'    (history walk failed: {e})')
