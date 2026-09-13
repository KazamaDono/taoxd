# ch03-arch/gotplt/Makefile — dynamic-linking lab: GOT/PLT + lazy binding.
#
# Builds greeter in four shapes so you can SEE what each toolchain choice does
# to the GOT, then proves the chapter's central claim (lst-tracegot): under
# lazy binding, GOT[puts] starts as a PLT resolver stub and is rewritten to
# point at libc after the first call.
#
# `make test` gates on four things, all deterministic on Ubuntu 24.04
# (x86-64 and arm64 runners):
#   1. trace_got.py        gdb+pwntools: GOT[puts] moves  <- the "it works" gate
#   2. observe_binding.sh  loader tracing: lazy vs eager, no debugger
#   3. readelf             the GOT shape each build produces (maps to tbl-relro)
#   4. greeter.arm .plt    the AArch64 adrp/ldr/br stub (Exercise 6)
SHELL := /bin/bash
PY    := python3

# Build aarch64 natively on an arm64 host, else with the cross toolchain.
UNAME_M := $(shell uname -m)
ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
  CC_ARM      := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)
  OBJDUMP_ARM := $(shell command -v aarch64-linux-gnu-objdump 2>/dev/null)
else
  CC_ARM      := gcc
  OBJDUMP_ARM := $(shell command -v objdump 2>/dev/null)
endif

# ---------------------------------------------------------------------------
# greeter (DEFAULT build — the one we trace): exactly how Ubuntu 24.04 ships a
# dynamically linked program, so the GOT behaves as the chapter describes.
#   PIE            -fPIE -pie             ET_DYN, random base (default)
#   NX             -Wl,-z,noexecstack     non-executable stack (default)
#   Partial RELRO  -Wl,-z,relro           NO -z now: .got.plt stays WRITABLE,
#                                         so lazy binding works and GOT[puts]
#                                         is resolved on first call.
#   Lazy binding   -Wl,-z,lazy            the default, pinned EXPLICITLY so the
#                                         trace stays reproducible even if a
#                                         future toolchain flips to -z now.
# -O0 keeps FORTIFY off (Ubuntu enables _FORTIFY_SOURCE only at -O1+), so the
# imports stay the plain `puts`/`printf` the chapter names, not `__*_chk`.
CFLAGS_DEFAULT  := -O0 -g -fPIE
LDFLAGS_DEFAULT := -pie -Wl,-z,relro -Wl,-z,lazy -Wl,-z,noexecstack

# greeter-fullrelro: FULL RELRO. `-z now` forces EAGER binding, then the loader
# re-maps the ENTIRE GOT read-only — a GOT overwrite becomes a segfault, not a
# primitive. This is the modern default later chapters must work around.
LDFLAGS_FULLRELRO := -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack

# greeter-nofplt: `-fno-plt` emits external calls as indirect calls THROUGH the
# GOT (no per-function PLT stub); it implies eager binding, so `puts` becomes a
# GLOB_DAT reloc resolved at load — not a lazy JUMP_SLOT.
CFLAGS_NOFPLT := -O0 -g -fPIE -fno-plt

NATIVE := greeter greeter-fullrelro greeter-nofplt

.DEFAULT_GOAL := all
.PHONY: all arm got test clean

all: $(NATIVE) arm
	@echo "built: $(NATIVE)$(if $(CC_ARM), greeter.arm,)"

greeter: greeter.c
	gcc $(CFLAGS_DEFAULT) $(LDFLAGS_DEFAULT) -o $@ $<
greeter-fullrelro: greeter.c
	gcc $(CFLAGS_DEFAULT) $(LDFLAGS_FULLRELRO) -o $@ $<
greeter-nofplt: greeter.c
	gcc $(CFLAGS_NOFPLT) $(LDFLAGS_FULLRELRO) -o $@ $<

# AArch64 greeter for the PLT-stub exercise. Native on arm64, cross on x86-64.
arm: greeter.arm
greeter.arm: greeter.c
ifeq ($(CC_ARM),)
	@echo "-- skipping greeter.arm (no aarch64 compiler on $(UNAME_M) host)"
else
	$(CC_ARM) $(CFLAGS_DEFAULT) $(LDFLAGS_DEFAULT) -o $@ $<
endif

# Convenience: show the GOT with pwndbg-style readelf relocs.
got: greeter
	@readelf -W -r greeter | sed -n '1,40p'

# ---------------------------------------------------------------------------
test: all
	@set -uo pipefail; fail=0; \
	echo "########## 1. lazy binding: GOT[puts] moves (trace_got.py) ##########"; \
	run_trace() { \
	  : > trace_got.log; \
	  if [ -n "$${TMUX:-}" ] || ! command -v tmux >/dev/null 2>&1; then \
	    $(PY) trace_got.py >trace_got.log 2>&1 || true; return 0; \
	  fi; \
	  tmux kill-session -t ch03trace 2>/dev/null || true; \
	  tmux new-session -d -s ch03trace "$(PY) trace_got.py >trace_got.log 2>&1"; \
	  i=0; \
	  while [ $$i -lt 240 ]; do \
	    grep -q "lazy binding confirmed" trace_got.log && break; \
	    grep -q "Traceback" trace_got.log && break; \
	    tmux has-session -t ch03trace 2>/dev/null || break; \
	    sleep 0.5; i=$$((i+1)); \
	  done; \
	  tmux kill-session -t ch03trace 2>/dev/null || true; \
	  return 0; \
	}; \
	run_trace; \
	cat trace_got.log 2>/dev/null || true; \
	if grep -q "lazy binding confirmed" trace_got.log; then \
	  echo "ok: GOT[puts] moved from the PLT stub to libc on first call"; \
	else echo "FAIL: trace_got.py did not confirm lazy binding (see log above)"; fail=1; fi; \
	echo; echo "########## 2. loader tracing: lazy vs eager (observe_binding.sh) ##########"; \
	bash observe_binding.sh | tee observe.log || true; \
	grep -q "symbol .puts'" observe.log || { echo "FAIL: no puts binding line from the loader"; fail=1; }; \
	grep -q "symbol .printf'" observe.log || { echo "FAIL: eager (LD_BIND_NOW) run did not bind printf"; fail=1; }; \
	echo; echo "########## 3. the GOT shape each build produces (tbl-relro) ##########"; \
	echo "-- greeter (partial RELRO, lazy): puts is a JUMP_SLOT --"; \
	if readelf -W -r greeter | grep JUMP_SLOT | grep -w puts; then :; \
	  else echo "FAIL: puts is not a lazy JUMP_SLOT in greeter"; fail=1; fi; \
	if readelf -W -d greeter | grep -qw BIND_NOW; then \
	  echo "FAIL: default greeter is unexpectedly BIND_NOW"; fail=1; \
	  else echo "ok: greeter is lazy (no BIND_NOW flag)"; fi; \
	echo "-- greeter-fullrelro (-z now): BIND_NOW set, eager --"; \
	if readelf -W -d greeter-fullrelro | grep -Ew 'BIND_NOW|NOW'; then :; \
	  else echo "FAIL: full-RELRO build lacks the BIND_NOW flag"; fail=1; fi; \
	echo "-- greeter-nofplt (-fno-plt): puts via GLOB_DAT, no lazy PLT slot --"; \
	if readelf -W -r greeter-nofplt | grep GLOB_DAT | grep -w puts; then :; \
	  else echo "FAIL: puts is not a GLOB_DAT under -fno-plt"; fail=1; fi; \
	if readelf -W -r greeter-nofplt | grep JUMP_SLOT | grep -qw puts; then \
	  echo "FAIL: -fno-plt build still has a puts JUMP_SLOT"; fail=1; \
	  else echo "ok: -fno-plt removed the puts PLT slot"; fi; \
	echo; echo "########## 4. AArch64 PLT stub adrp/ldr/br (Exercise 6) ##########"; \
	if [ -f greeter.arm ] && [ -n "$(OBJDUMP_ARM)" ]; then \
	  $(OBJDUMP_ARM) -d -j .plt greeter.arm > plt.arm.txt 2>/dev/null || true; \
	  if grep -qw adrp plt.arm.txt && grep -qw ldr plt.arm.txt && grep -qw br plt.arm.txt; then \
	    echo "ok: greeter.arm .plt uses the adrp/ldr/br sequence (via x16/x17)"; \
	  else echo "FAIL: greeter.arm .plt missing the adrp/ldr/br stub"; fail=1; fi; \
	else echo "-- no greeter.arm or aarch64 objdump; skipping the AArch64 PLT check"; fi; \
	echo; \
	if [ $$fail -ne 0 ]; then echo "ch03-arch/gotplt: make test FAILED"; exit 1; fi; \
	echo "ch03-arch/gotplt: all checks passed — make test OK"

clean:
	rm -f $(NATIVE) greeter.arm *.o *.log plt.arm.txt .trace.rc
	rm -rf __pycache__
