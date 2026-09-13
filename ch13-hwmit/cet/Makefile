# Chapter 13 — CET demos (Intel/AMD x86-64).
#
# Mitigation flags:
#   -fcf-protection=full : turns on BOTH shadow-stack (SHSTK) landing pads and
#                          Indirect Branch Tracking (IBT / ENDBR64). This is
#                          the toggle that makes the demos observably fault
#                          on CET-capable silicon (Tiger Lake+/Zen 3+).
#   -O2                  : realistic optimization; the frame layout the demo
#                          reads with `lea 8(%rbp), %0` still holds.
#
# The `test` target builds both demos and, when GLIBC_TUNABLES lets SHSTK run,
# invokes run.sh which greps for the unique success marker printed by our
# SIGSEGV handler. On non-CET hardware we exit 0 without asserting the fault:
# the CI runner may not carry the silicon.

CC       ?= gcc
CFLAGS   ?= -O2 -fno-omit-frame-pointer -fcf-protection=full -Wall -Wextra
TARGETS   = shstk_demo ibt_demo

.PHONY: all test clean
all: $(TARGETS)

shstk_demo: shadow_stack_demo.c
	$(CC) $(CFLAGS) $< -o $@

ibt_demo: ibt_demo.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	@bash ./run.sh

clean:
	rm -f $(TARGETS)
