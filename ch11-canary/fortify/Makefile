# ch11-canary/fortify/Makefile
#
# Build demo.c THREE times, once per FORTIFY_SOURCE level, and disassemble
# copy_dynamic at each so exercise 5 (which level does __strcpy_chk first
# appear at?) has evidence you can read.
#
# Shared flags:
#   -O2                : FORTIFY needs optimisation to be meaningful --
#                        without it the header's __fortify_function inline
#                        wrappers are never inlined and every call becomes
#                        the raw libc symbol.
#   -Wall -g           : usual.
#   -no-pie            : keeps disassembly addresses stable.
#   -U_FORTIFY_SOURCE  : clear any inherited default before setting our own.
#
# One flag decides everything:
#   -D_FORTIFY_SOURCE=1 : "easy case" -- compile-time-known dst AND len.
#   -D_FORTIFY_SOURCE=2 : plus %n-in-writable-format rejection, tighter
#                          checks on some runtime-known lengths.
#   -D_FORTIFY_SOURCE=3 : Ubuntu 24.04 default. Uses
#                          __builtin_dynamic_object_size to bound
#                          malloc(n)-shaped destinations -- the whole reason
#                          copy_dynamic gets a checker at this level and not
#                          the earlier ones.

SHELL := /bin/bash
CC    ?= gcc

CFLAGS_BASE = -O2 -Wall -g -no-pie -U_FORTIFY_SOURCE

.DEFAULT_GOAL := all
.PHONY: all test clean

all: demo_f1 demo_f2 demo_f3

demo_f1: demo.c
	$(CC) $(CFLAGS_BASE) -D_FORTIFY_SOURCE=1 -o $@ demo.c

demo_f2: demo.c
	$(CC) $(CFLAGS_BASE) -D_FORTIFY_SOURCE=2 -o $@ demo.c

demo_f3: demo.c
	$(CC) $(CFLAGS_BASE) -D_FORTIFY_SOURCE=3 -o $@ demo.c

# The exercise: which level does __strcpy_chk first appear at for
# copy_dynamic? Print the disassembly at all three levels, then ASSERT the
# two claims Chapter 11 makes:
#   (positive control) copy_static gets __*_chk at level 1 already
#   (main claim)       copy_dynamic gets __*_chk at level 3
test: all
	@echo "== Disassembly of copy_dynamic at each FORTIFY level =="
	@for lvl in 1 2 3; do \
	  echo "--- _FORTIFY_SOURCE=$$lvl ---"; \
	  objdump -d --disassemble=copy_dynamic demo_f$$lvl 2>/dev/null \
	    | grep -E 'call|_chk' || echo "(no fortified call in copy_dynamic)"; \
	done
	@echo "== positive control: copy_static must be fortified at _F_S=1 =="
	@objdump -d --disassemble=copy_static demo_f1 2>/dev/null | grep -q '_chk' \
	  || { echo "FAIL: expected some __*_chk call in copy_static at _F_S=1"; exit 1; }
	@echo "== main claim: copy_dynamic must be fortified at _F_S=3 =="
	@objdump -d --disassemble=copy_dynamic demo_f3 2>/dev/null | grep -q '_chk' \
	  || { echo "FAIL: expected __*_chk in copy_dynamic at _F_S=3"; exit 1; }
	@echo "ch11-canary/fortify: OK"

clean:
	rm -f demo_f1 demo_f2 demo_f3
