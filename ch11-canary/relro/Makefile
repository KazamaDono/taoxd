# ch11-canary/relro/Makefile
#
# Same relro_demo.c, built TWO ways so the RELRO lesson is the diff.
#
# Shared flags:
#   -no-pie                  : fixed base; the exploit needs puts@got.plt at
#                              a known constant.
#   -fno-stack-protector     : we are not attacking the stack in this lab.
#   -U_FORTIFY_SOURCE
#   -D_FORTIFY_SOURCE=0      : keep FORTIFY out of this lab's story.
#   -Wl,-z,noexecstack       : NX ON.
#   -O1                      : stable code gen.
#
# The one line that decides everything:
#   relro_partial :  -Wl,-z,relro                       (.got.plt R/W)
#   relro_full    :  -Wl,-z,relro  -Wl,-z,now           (.got.plt R/O)
#
# `-z,now` disables lazy binding: the loader resolves every dynamic symbol
# at startup and then mprotect()s .got.plt read-only. That is why the
# arbitrary-write primitive in relro_demo.c segfaults on the write itself
# under relro_full, while it happily hijacks puts under relro_partial.

SHELL := /bin/bash
CC    ?= gcc
PY    ?= python3

CFLAGS = -O1 -Wall -fno-stack-protector -no-pie -g \
         -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

INCLUDES = -I../../common

RELRO_PARTIAL_LDFLAGS = -Wl,-z,relro          -Wl,-z,noexecstack
RELRO_FULL_LDFLAGS    = -Wl,-z,relro,-z,now   -Wl,-z,noexecstack

.DEFAULT_GOAL := all
.PHONY: all test clean checksec

all: relro_partial relro_full

relro_partial: relro_demo.c ../../common/harness.h
	$(CC) $(CFLAGS) $(INCLUDES) $(RELRO_PARTIAL_LDFLAGS) -o $@ relro_demo.c

relro_full: relro_demo.c ../../common/harness.h
	$(CC) $(CFLAGS) $(INCLUDES) $(RELRO_FULL_LDFLAGS)    -o $@ relro_demo.c

# CI proof: the SAME solve script wins against partial and faults against
# full. Both outcomes are asserted -- either surprise (partial fails, or
# full succeeds) exits non-zero.
test: all
	@command -v $(PY) >/dev/null 2>&1 || { echo "FAIL: $(PY) missing"; exit 1; }
	@$(PY) -c 'import pwn' 2>/dev/null || { echo "FAIL: pwntools missing"; exit 1; }
	@echo "== [1/2] relro_partial : expect hijack success =="
	@timeout 20 $(PY) solve.py partial ./relro_partial
	@echo "== [2/2] relro_full    : expect SIGSEGV on the write =="
	@timeout 20 $(PY) solve.py full    ./relro_full
	@echo "ch11-canary/relro: OK"

checksec: all
	@for b in relro_partial relro_full; do \
	  echo "== $$b =="; \
	  pwn checksec $$b 2>/dev/null || checksec --file=$$b 2>/dev/null || \
	    readelf -lW $$b | grep -E 'GNU_RELRO|GNU_STACK'; \
	done

clean:
	rm -f relro_partial relro_full pwn.marker
