# ch32-symbolic/Makefile
#
# Builds the two lab targets in the two dual-arch flavours the book uses,
# and runs the angr solvers under pytest.  CI runs `make test`.
#
# ---- Mitigation posture (documented per target) -----------------------------
#
# xorcheck: this is a crackme, not a memory-safety target.  It is compiled
# with a *modern default posture* — PIE + full RELRO + stack protector +
# NX — because the point of the exercise is to prove that a symbolic
# solver walks straight through those defenses (they do not stop a
# reasoning attack).  `-fno-inline` keeps `check`, `win`, `lose` as
# distinct symbols so the angr script can hook them by name.
#
# parser: deliberately vulnerable (signed/unsigned confusion at do_copy).
# We turn OFF the stack protector (`-fno-stack-protector`) so the SIGSEGV
# is reached via the vulnerable memcpy rather than being converted into
# a __stack_chk_fail abort.  FORTIFY is disabled (`-U_FORTIFY_SOURCE`)
# so glibc's memcpy overflow check does not intercept the copy.  PIE and
# RELRO stay ON because they do not affect reachability of the bug and we
# want the target to look like a real modern binary otherwise.  The bug
# is real; the mitigations exposed here are the ones the vuln evades.
# ---------------------------------------------------------------------------

CC      ?= gcc
AARCH64_CC ?= aarch64-linux-gnu-gcc
PYTHON  ?= python3

XOR_CFLAGS = -O2 -std=c11 -Wall -Wextra -fno-inline \
             -fPIE -pie -fstack-protector-strong \
             -Wl,-z,relro,-z,now,-z,noexecstack
PARSER_CFLAGS = -O0 -g -std=c11 -Wall -Wextra -fno-inline \
                -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
                -fPIE -pie -Wl,-z,relro,-z,now,-z,noexecstack

.PHONY: all x86 arm64 clean test venv deps

all: x86

x86: xorcheck-x86_64 parser-x86_64
arm64: xorcheck-arm64 parser-arm64

xorcheck-x86_64: src/xorcheck.c
	$(CC) $(XOR_CFLAGS) -o $@ $<

parser-x86_64: src/parser.c
	$(CC) $(PARSER_CFLAGS) -o $@ $<

xorcheck-arm64: src/xorcheck.c
	$(AARCH64_CC) $(XOR_CFLAGS) -o $@ $<

parser-arm64: src/parser.c
	$(AARCH64_CC) $(PARSER_CFLAGS) -o $@ $<

# --- test rig ---------------------------------------------------------------
#
# angr is a heavyweight Python dependency; we install it into a chapter-local
# virtualenv so the CI job can `make test` without polluting the system Python.
VENV = .venv
PY   = $(VENV)/bin/python
PIP  = $(VENV)/bin/pip

$(VENV)/bin/activate:
	$(PYTHON) -m venv $(VENV)
	$(PIP) install --upgrade pip wheel

deps: $(VENV)/bin/activate
	$(PIP) install -r requirements.txt

# `make test` builds x86 targets, installs angr, and runs pytest.
# Exits 0 ONLY if both solver end-to-ends succeed (they grep the
# "SOLVED" marker from win() and check for a segfault return code).
test: x86 deps
	$(VENV)/bin/pytest -q tests/test_solvers.py

clean:
	rm -f xorcheck-x86_64 parser-x86_64 xorcheck-arm64 parser-arm64
	rm -rf $(VENV) .pytest_cache **/__pycache__
