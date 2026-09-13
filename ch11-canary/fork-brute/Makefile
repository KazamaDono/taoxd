# ch11-canary/fork-brute/Makefile
#
# Build the fork-model target and prove the byte-by-byte canary brute.
#
# Mitigation cocktail (each flag has a reason):
#   -fstack-protector-strong  : the whole point -- every child inherits the
#                               PARENT'S canary via fork(), which is exactly
#                               the crack the brute widens.
#   -U_FORTIFY_SOURCE
#   -D_FORTIFY_SOURCE=0       : OFF on purpose. FORTIFY does NOT wrap recv()
#                               with an attacker-controlled length anyway
#                               (see ../fortify's demo.c case C), but pin it
#                               off so the linker cannot confuse things.
#   -Wl,-z,relro,-z,now       : full RELRO. We attack the RETURN ADDRESS,
#                               not the GOT.
#   -no-pie                   : fixed image base -- win()'s address is a
#                               known constant in the exploit payload.
#   -Wl,-z,noexecstack        : NX ON. Code reuse, not shellcode.
#   -O1                       : stable frame layout for teaching.

SHELL := /bin/bash
CC    ?= gcc
PY    ?= python3

CFLAGS  = -O1 -Wall -fstack-protector-strong \
          -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
          -no-pie -g
LDFLAGS = -Wl,-z,relro,-z,now -Wl,-z,noexecstack

INCLUDES = -I../../common

.DEFAULT_GOAL := all
.PHONY: all run test clean checksec

all: server

server: server.c ../../common/harness.h
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ server.c

# Run the server in the foreground (useful for manual solve.py runs).
run: server
	./server

# Deterministic end-to-end: start the server, run solve.py, assert marker.
# The port (4011) is bound to 127.0.0.1 only, so CI's shared runners are safe.
# `setarch -R` keeps ASLR off for reproducibility (the parent process is
# what the children inherit, so ASLR affects the canary value but not the
# brute technique -- pinned for determinism, not required).
test: server
	@rm -f pwn.marker server.log
	@command -v $(PY) >/dev/null 2>&1 || { echo "FAIL: $(PY) missing"; exit 1; }
	@$(PY) -c 'import pwn' 2>/dev/null || { echo "FAIL: pwntools missing"; exit 1; }
	@if command -v setarch >/dev/null 2>&1 && setarch -R /bin/true >/dev/null 2>&1; then RUN="setarch -R"; else RUN=""; fi; \
	  echo "== starting fork-brute server on 127.0.0.1:4011 =="; \
	  $$RUN ./server >server.log 2>&1 & \
	  echo $$! > server.pid; \
	  trap 'kill $$(cat server.pid) 2>/dev/null; rm -f server.pid' EXIT; \
	  for i in $$(seq 1 40); do \
	    if ss -ltn 2>/dev/null | grep -q :4011 || \
	       (echo > /dev/tcp/127.0.0.1/4011) 2>/dev/null; then break; fi; \
	    sleep 0.1; \
	  done; \
	  echo "== brute + reproduce canary + hijack ret =="; \
	  timeout 120 $(PY) solve.py; rc=$$?; \
	  kill $$(cat server.pid) 2>/dev/null; rm -f server.pid; \
	  test $$rc -eq 0 || { echo "FAIL: solve.py exit $$rc"; cat server.log; exit 1; }
	@test -f pwn.marker || { echo "FAIL: pwn.marker not written"; exit 1; }
	@echo "ch11-canary/fork-brute: OK"

checksec: server
	@pwn checksec server 2>/dev/null || checksec --file=server 2>/dev/null || \
	  readelf -lW server | grep -E 'GNU_STACK|GNU_RELRO'

clean:
	rm -f server pwn.marker server.log server.pid
