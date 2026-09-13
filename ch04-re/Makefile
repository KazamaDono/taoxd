# ch04-re/Makefile — build the lab crackme (symbolized + stripped + AArch64)
# and prove the solver recovers a key the binary accepts.
#
# CI-runnable on Ubuntu 24.04 (both the x86-64 and the arm64 runner):
#   * x86-64 host : crackme-x86_64[.stripped] build natively; crackme-arm64
#                   is cross-built with aarch64-linux-gnu-gcc and run under
#                   qemu-aarch64-static.
#   * arm64  host : crackme-arm64[.stripped] build natively; the x86-64
#                   variants are skipped unless an x86_64 cross-gcc is present.
# `make test` runs solve/solve.py against every binary it could build and
# exits non-zero unless each one prints the success string ("Correct!").
#
# This solve is address-independent (it inverts a constant table, then runs
# the binary with the recovered key), so NO ASLR-disabling / setarch -R is
# needed — the test is deterministic with mitigations fully on.

SHELL := /bin/bash
PY    := python3
SRC   := src/crackme.c
SOLVE := solve/solve.py

# ---------------------------------------------------------------------------
# Mitigations — ALL ON, because this is a *modern-default* build, not a toy.
# The chapter's `checksec` output (lst-triage) shows exactly these five:
#
#   PIE          -fPIE -pie                 ET_DYN, random load base
#   NX           -Wl,-z,noexecstack         non-executable stack (W^X)
#   Full RELRO   -Wl,-z,relro -Wl,-z,now    GOT resolved + mapped RO at startup
#   Stack canary -fstack-protector-all      see the note below
#   FORTIFY      -D_FORTIFY_SOURCE=3        fortified libc wrappers (needs -O>=1);
#                                           =3 is the Noble/glibc-2.39 default and
#                                           the stack we pin (see bible/tech-stack.md)
#
# NOTE on the canary: the distro default is -fstack-protector-strong, but
# neither check_serial nor main has a stack *buffer* or an address-taken
# local, so -strong would emit no canary at all and `checksec` would read
# "No canary found" — contradicting the chapter. -fstack-protector-all puts a
# canary (the fs:0x28 / __stack_chk_fail signature from the "compiler idioms"
# section) in every function, so the defense is demonstrably present exactly
# as lst-triage shows. lst-decomp is "lightly cleaned" of that standard epilogue.
#
# NOTE on -O1 -fno-inline: check_serial is `static` and the entire chapter
# depends on it surviving as its own symbol (nm shows `t check_serial`, gdb
# breaks on it, Frida hooks it by name). -fno-inline guarantees the optimizer
# keeps it out-of-line; -O1 keeps the decompiler output as clean as lst-decomp
# while remaining a realistic optimized build. (lst-asm is explicitly
# "representative -O1", not lifted from these exact bytes.)
CFLAGS  := -std=c11 -O1 -fno-inline -Wall -Wextra \
           -fPIE -fstack-protector-all -D_FORTIFY_SOURCE=3
LDFLAGS := -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

# ---------------------------------------------------------------------------
# Toolchain selection: build whatever THIS host can target.
UNAME_M := $(shell uname -m)

# --- a compiler (+ qemu runner) that targets x86-64 ------------------------
ifeq ($(UNAME_M),x86_64)
  CC_X86    := gcc
  STRIP_X86 := strip
  RUN_X86   :=
else
  CC_X86    := $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null)
  STRIP_X86 := x86_64-linux-gnu-strip
  RUN_X86   := qemu-x86_64-static -L /usr/x86_64-linux-gnu
endif

# --- a compiler (+ qemu runner) that targets aarch64 -----------------------
ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
  CC_ARM    := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)
  STRIP_ARM := aarch64-linux-gnu-strip
  RUN_ARM   := qemu-aarch64-static -L /usr/aarch64-linux-gnu
else
  CC_ARM    := gcc
  STRIP_ARM := strip
  RUN_ARM   :=
endif

# ---------------------------------------------------------------------------
# What we can build on this host.
BINS :=
ifneq ($(CC_X86),)
  BINS += crackme-x86_64 crackme-x86_64.stripped
endif
ifneq ($(CC_ARM),)
  BINS += crackme-arm64 crackme-arm64.stripped
endif

# Per-arch run plans (computed at parse time so the big test recipe stays one
# shell). Each clause ends in ';' so it concatenates cleanly.
ifneq ($(CC_X86),)
  ifeq ($(UNAME_M),x86_64)
    X86_RUNS := echo "== x86-64 (native) =="; run_one crackme-x86_64 ""; run_one crackme-x86_64.stripped "";
  else ifneq ($(shell command -v qemu-x86_64-static 2>/dev/null),)
    X86_RUNS := echo "== x86-64 (under qemu) =="; run_one crackme-x86_64 "$(RUN_X86)"; run_one crackme-x86_64.stripped "$(RUN_X86)";
  else
    X86_RUNS := echo "-- built x86-64 but qemu-x86_64-static missing; skipping its run";
  endif
else
  X86_RUNS := echo "-- skipping x86-64 (no x86-64 compiler on $(UNAME_M) host)";
endif

ifneq ($(CC_ARM),)
  ifneq ($(filter aarch64 arm64,$(UNAME_M)),)
    ARM_RUNS := echo "== aarch64 (native) =="; run_one crackme-arm64 ""; run_one crackme-arm64.stripped "";
  else ifneq ($(shell command -v qemu-aarch64-static 2>/dev/null),)
    ARM_RUNS := echo "== aarch64 (under qemu) =="; run_one crackme-arm64 "$(RUN_ARM)"; run_one crackme-arm64.stripped "$(RUN_ARM)";
  else
    ARM_RUNS := echo "-- built aarch64 but qemu-aarch64-static missing; skipping its run";
  endif
else
  ARM_RUNS := echo "-- skipping aarch64 (no aarch64 compiler on $(UNAME_M) host)";
endif

.DEFAULT_GOAL := all
.PHONY: all test checksec clean

all: $(BINS)
	@echo "built: $(BINS)"

# --- build rules -----------------------------------------------------------
crackme-x86_64: $(SRC)
	$(CC_X86) $(CFLAGS) $(LDFLAGS) -o $@ $<

crackme-x86_64.stripped: crackme-x86_64
	cp -f $< $@
	$(STRIP_X86) --strip-all $@          # drops .symtab; dynamic imports survive

crackme-arm64: $(SRC)
	$(CC_ARM) $(CFLAGS) $(LDFLAGS) -o $@ $<

crackme-arm64.stripped: crackme-arm64
	cp -f $< $@
	$(STRIP_ARM) --strip-all $@

# --- the "it works" gate ---------------------------------------------------
# Builds everything buildable, then runs the solver against each binary.
# A foreign-arch binary is run through a tiny qemu wrapper so the exact,
# book-listed solve.py (which just does process([binary, key])) stays unchanged.
test: all
	@set -euo pipefail; \
	if [ -z "$(strip $(BINS))" ]; then \
	  echo "ch04-re: no crackme variant could be built on $(UNAME_M) — install a compiler" >&2; exit 1; \
	fi; \
	run_one() { \
	  bin="$$1"; runner="$$2"; \
	  if [ ! -x "$$bin" ]; then echo "ERROR: $$bin not built" >&2; exit 1; fi; \
	  echo "---- solving $$bin ----"; \
	  if [ -n "$$runner" ]; then \
	    printf '#!/bin/sh\nexec %s "%s" "$$@"\n' "$$runner" "$$PWD/$$bin" > "$$bin.run"; \
	    chmod +x "$$bin.run"; \
	    $(PY) $(SOLVE) "./$$bin.run" || { echo "FAIL: $$bin (under qemu)" >&2; exit 1; }; \
	  else \
	    $(PY) $(SOLVE) "./$$bin" || { echo "FAIL: $$bin" >&2; exit 1; }; \
	  fi; \
	}; \
	$(X86_RUNS) \
	$(ARM_RUNS) \
	echo; echo "ch04-re: all built crackme variants solved — make test OK"

# --- convenience: show the mitigations the chapter triages -----------------
checksec: $(BINS)
	@for b in $(BINS); do \
	  echo "== $$b =="; \
	  pwn checksec "$$b" 2>/dev/null || checksec --file="$$b" 2>/dev/null || \
	    echo "(install pwntools or checksec to see the mitigation table)"; \
	done

clean:
	rm -f crackme-x86_64 crackme-x86_64.stripped crackme-arm64 crackme-arm64.stripped *.run
	rm -rf solve/__pycache__ __pycache__
