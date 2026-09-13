# ch11-canary/canary/Makefile
#
# Build a deliberately vulnerable single-connection target with the stack
# canary ON (that is the defence the whole chapter is about) and the other
# mitigations pinned so the exploit has a single moving part: the canary.
#
# Flags, with the "why":
#   -fstack-protector-strong  : install the per-thread canary in every
#                               function with a stack buffer -- exactly what
#                               Ubuntu 24.04 ships as the default. The
#                               ENTIRE point of this lab is defeating it.
#   -U_FORTIFY_SOURCE
#   -D_FORTIFY_SOURCE=0       : OFF on purpose. FORTIFY would rewrite our
#                               printf(user_fmt) into __printf_chk() and
#                               reject the leak primitive -- the FORTIFY
#                               lab (../fortify) covers that mitigation on
#                               its own terms.
#   -Wl,-z,relro,-z,now       : full RELRO. We are NOT attacking the GOT
#                               here (RELRO gets its own lab in ../relro),
#                               so lock it down and keep this target focused.
#   -no-pie                   : fixed image base -- win() has a stable
#                               address, so the payload after the reproduced
#                               canary points at a known constant.
#   -Wl,-z,noexecstack        : NX ON. We reuse existing code, no shellcode.
#   -Wno-format-security      : silence the (correct) printf(user_fmt)
#                               warning; the bug is the whole listing.
#   -O1                       : stable frame layout for teaching -- avoids
#                               -O0 dead-code and -O2 inlining shenanigans.

SHELL := /bin/bash
CC    ?= gcc
PY    ?= python3

CFLAGS  = -O1 -Wall -Wno-format-security -fstack-protector-strong \
          -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
          -no-pie -g
LDFLAGS = -Wl,-z,relro,-z,now -Wl,-z,noexecstack

INCLUDES = -I../../common

.DEFAULT_GOAL := all
.PHONY: all arm64 test clean checksec

all: leak

leak: leak.c ../../common/harness.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ leak.c

# Best-effort AArch64 build under qemu-user. Never fails the top-level build:
# CI's arm64 runner is the real gate. Requires the aarch64 cross toolchain
# (installed in the lab image; may be absent on developer machines).
arm64:
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
	  aarch64-linux-gnu-gcc $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o leak.aarch64 leak.c \
	  && echo "built leak.aarch64 (run with 'qemu-aarch64 -L /usr/aarch64-linux-gnu leak.aarch64')"; \
	else echo "-- skipping aarch64: no aarch64-linux-gnu-gcc"; fi

# Deterministic proof. setarch -R disables ASLR for the child so the run is
# reproducible (win()'s address is a fixed constant either way, but the
# canary leak's stack offsets are stabler without ASLR jitter).
test: leak
	@rm -f pwn.marker
	@command -v $(PY) >/dev/null 2>&1 || { echo "FAIL: $(PY) missing"; exit 1; }
	@$(PY) -c 'import pwn' 2>/dev/null || { echo "FAIL: pwntools missing (pip install pwntools)"; exit 1; }
	@if command -v setarch >/dev/null 2>&1 && setarch -R /bin/true >/dev/null 2>&1; then RUN="setarch -R"; else RUN=""; fi; \
	  echo "== canary leak + reproduction =="; \
	  $$RUN timeout 20 $(PY) solve.py
	@test -f pwn.marker || { echo "FAIL: no pwn.marker written"; exit 1; }
	@echo "ch11-canary/canary: OK"

checksec: leak
	@pwn checksec leak 2>/dev/null || checksec --file=leak 2>/dev/null || \
	  readelf -lW leak | grep -E 'GNU_STACK|GNU_RELRO'

clean:
	rm -f leak leak.aarch64 pwn.marker
