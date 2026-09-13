"""End-to-end tests for the Chapter 32 symbolic-execution labs.

Assumes the Makefile has already built ./xorcheck-x86_64 and
./parser-x86_64 in the chapter directory (that is what `make test` does
before invoking pytest).
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CHDIR = os.path.abspath(os.path.join(HERE, os.pardir))


def _run(cmd, **kw):
    return subprocess.run(cmd, cwd=CHDIR, capture_output=True, **kw)


def test_xorcheck_solver_recovers_valid_key():
    r = _run([sys.executable, 'solve/solve_xorcheck.py', './xorcheck-x86_64'],
             timeout=180)
    assert r.returncode == 0, r.stderr.decode(errors='replace')
    line = r.stdout.decode().strip().splitlines()[-1]
    assert line.startswith('key:'), line
    key = line.split(':', 1)[1].strip()
    assert len(key) == 8, f'expected 8-char key, got {key!r}'
    # Feed the recovered key back to the binary — it MUST accept it.
    r2 = _run(['./xorcheck-x86_64', key], timeout=10)
    assert r2.returncode == 0, r2.stderr.decode(errors='replace')
    assert b'SOLVED' in r2.stdout, r2.stdout


def test_find_crash_produces_segfaulting_input():
    r = _run([sys.executable, 'solve/find_crash.py', './parser-x86_64'],
             timeout=180)
    assert r.returncode == 0, r.stderr.decode(errors='replace')
    poc = r.stdout
    assert poc.startswith(b'PKT'), poc[:8]
    # Feed poc to the parser under a resource-limited shell so a wild
    # memcpy exits cleanly instead of thrashing.
    r2 = subprocess.run(['./parser-x86_64'], input=poc, cwd=CHDIR,
                         capture_output=True, timeout=10)
    # A negative-cast length causes SIGSEGV -> returncode -11.
    assert r2.returncode < 0 or r2.returncode >= 128, (
        f'expected crash, got rc={r2.returncode} stdout={r2.stdout!r}')
