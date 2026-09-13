# ch11-canary/Makefile -- top-level entry point for Chapter 11's labs.
# CI runs `make -C ch11-canary test`; this delegates to test.sh which
# builds and tests each sub-lab (canary, fork-brute, relro, fortify).
SHELL := /bin/bash

SUBDIRS = canary fork-brute relro fortify

.DEFAULT_GOAL := all
.PHONY: all test clean checksec $(SUBDIRS)

all:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d all || exit 1; done

test:
	@bash ./test.sh

checksec:
	@for d in canary fork-brute relro; do \
	  echo "==== $$d ===="; \
	  $(MAKE) -C $$d checksec 2>/dev/null || true; \
	done

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
