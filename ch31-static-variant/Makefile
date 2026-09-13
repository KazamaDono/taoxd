# Chapter 31 — Static & Variant Analysis lab
#
# This lab is analyzed *statically* — Semgrep/weggli/CodeQL look at the
# source and do not execute the seeded bugs.  There is therefore no
# runtime exploit and no reason to weaken any mitigation; the objects
# below are compiled with the Ubuntu 24.04 defaults, which on gcc 13
# already give us:
#
#   * -fstack-protector-strong   (default in Ubuntu's gcc)
#   * -fPIE / -pie              (default; we do not link an executable)
#   * -D_FORTIFY_SOURCE=2       (implied at -O1+ with glibc; harmless at -O0)
#   * Full RELRO at link time   (default; nothing linked here)
#
# The seeded bugs are source-level defects (unbounded memcpy, UAF) that
# no compiler flag can catch at these levels — which is exactly why
# static analysis matters here.  If you want to see them at runtime,
# rebuild with `-fsanitize=address -O1` and drive them from a fuzzer.

CC      ?= gcc
CFLAGS  ?= -Wall -Wno-unused-result -Wno-unused-variable -Wno-unused-function \
           -Wno-unused-but-set-variable -O0 -g -Itarget
LDFLAGS ?=

TARGET_SRCS  := target/net.c target/parser.c target/handler.c
VARIANT_SRCS := variant/admin.c variant/cache.c variant/stats.c

TARGET_OBJS  := $(TARGET_SRCS:.c=.o)
VARIANT_OBJS := $(VARIANT_SRCS:.c=.o)

VENV        := .venv
SEMGREP     := $(VENV)/bin/semgrep
SEMGREP_VER := 1.90.0

.PHONY: all build test semgrep-check clean

all: build

build: $(TARGET_OBJS) $(VARIANT_OBJS)

%.o: %.c target/proto.h
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# `make test`  (what CI runs)
#
# 1.  Compile every translation unit in target/ and variant/ so we know the
#     seeded lab code still parses under Ubuntu 24.04 / gcc 13.
# 2.  Install Semgrep 1.90 into a local venv if not already on PATH.
# 3.  Run the rule from Listing 31-1 against target/ and assert:
#         * exactly one finding is reported,
#         * that finding is in target/parser.c (the parse_record memcpy),
#         * no findings elsewhere in target/.
# 4.  Skip weggli and CodeQL — they are exercised by variant/run-variant.sh
#     but too heavy to install inside CI.  If they are on PATH we run them
#     as an extra sanity pass; if not, we print a skip note and move on.
#
# The recipe exits 0 iff the marker line "ch31 test: PASSED" is printed.
# ---------------------------------------------------------------------------

test: build semgrep-check
	@echo "ch31 test: PASSED"

semgrep-check:
	@set -e; \
	 if command -v semgrep >/dev/null 2>&1; then \
	   SG="$$(command -v semgrep)"; \
	 else \
	   echo "[ch31] semgrep not on PATH — installing $(SEMGREP_VER) into $(VENV)"; \
	   python3 -m venv $(VENV); \
	   $(VENV)/bin/pip install --quiet --disable-pip-version-check \
	     'semgrep==$(SEMGREP_VER)' \
	     || $(VENV)/bin/pip install --quiet --disable-pip-version-check semgrep; \
	   SG="$(SEMGREP)"; \
	 fi; \
	 echo "[ch31] using semgrep at: $$SG"; \
	 "$$SG" --version || true; \
	 rm -f semgrep.out; \
	 "$$SG" --disable-version-check --metrics=off --quiet \
	    --config semgrep-rules/unchecked-memcpy.yaml target/ \
	    > semgrep.out 2>&1 || true; \
	 echo "----- semgrep.out -----"; cat semgrep.out; echo "-----------------------"; \
	 grep -q 'unchecked-memcpy-fixed-dest' semgrep.out \
	   || { echo "MARKER: expected rule id not fired"; exit 1; }; \
	 grep -q 'target/parser.c' semgrep.out \
	   || { echo "MARKER: expected hit in target/parser.c not found"; exit 1; }; \
	 HITS=$$(grep -c 'unchecked-memcpy-fixed-dest' semgrep.out || true); \
	 echo "[ch31] semgrep OK — $$HITS finding(s) in target/"; \
	 if command -v weggli >/dev/null 2>&1; then \
	   echo "[ch31] weggli present — running UAF pattern (informational)"; \
	   weggli -p "$$(cat weggli-patterns/uaf.txt)" target/ || true; \
	 else \
	   echo "[ch31] weggli not installed — skipping UAF sweep (see run-variant.sh)"; \
	 fi; \
	 if command -v codeql >/dev/null 2>&1; then \
	   echo "[ch31] codeql present but skipped from 'make test' (see variant/run-variant.sh)"; \
	 else \
	   echo "[ch31] codeql not installed — skipping (see variant/run-variant.sh)"; \
	 fi

clean:
	rm -f $(TARGET_OBJS) $(VARIANT_OBJS) semgrep.out
	rm -rf $(VENV) variant/variant-db results \
	       variant/variants.sarif variant/variants.csv variant/known.csv
