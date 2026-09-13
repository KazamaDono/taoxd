# ch03-arch/Makefile — architecture lab for Chapter 3.
#
# Builds the three observation targets natively, and (with `make arm`) their
# AArch64 cross-compiled twins, then proves the chapter's mechanical claims:
#   * triangle / args8 compute the values the prose predicts (exit status)
#   * aslr_map prints every major region base, and those bases FREEZE under
#     `setarch -R` (the ASLR-off determinism the book leans on for labs)
#   * the x86-64 `pack` reads its 7th/8th args off the STACK while the AArch64
#     `pack` reads none (the six-vs-eight arg-register difference, Exercise 2)
#   * compare_arch.sh renders the two ISAs side by side (x86-64 host)
#   * gotplt/ proves lazy binding moves GOT[puts] (recursed)
#
# CI-runnable on Ubuntu 24.04, both the x86-64 and the arm64 runner.
SHELL := /bin/bash

# ---------------------------------------------------------------------------
# Mitigations for the observation targets: MODERN DEFAULTS, all ON, nothing
# disabled. Chapter 3 is about READING present-day binaries, not corrupting
# them, so these are built exactly as Ubuntu 24.04 ships code:
#   PIE           -fPIE -pie              ET_DYN, random load base — this is the
#                                         very thing aslr_map watches move.
#   NX            -Wl,-z,noexecstack      non-executable stack (W^X).
#   Partial RELRO -Wl,-z,relro            (no -z now) — the distro default.
# -O1 matches the codegen the chapter disassembles (fig-disasm is "-O1").
CFLAGS  := -O1 -g -Wall -Wextra -fPIE
LDFLAGS := -pie -Wl,-z,relro -Wl,-z,noexecstack

# ---------------------------------------------------------------------------
# Toolchain: build aarch64 natively on an arm64 host, else via the cross gcc.
UNAME_M := $(shell uname -m)
ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
  CC_ARM  := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)
  QEMU_ARM := $(shell command -v qemu-aarch64-static 2>/dev/null)
  RUN_ARM := $(QEMU_ARM) -L /usr/aarch64-linux-gnu
else
  CC_ARM  := gcc
  QEMU_ARM := native
  RUN_ARM :=
endif

NATIVE := triangle args8 aslr_map
ARMBIN := triangle.arm args8.arm aslr_map.arm

.DEFAULT_GOAL := all
.PHONY: all arm test clean

all: $(NATIVE)
	@echo "built (native): $(NATIVE)"

# native observation targets
triangle: triangle.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<
args8: args8.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<
aslr_map: aslr_map.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $<

# AArch64 cross-compiled twins (`make arm`). Skipped if no aarch64 compiler.
arm: $(ARMBIN)
triangle.arm args8.arm aslr_map.arm: %.arm: %.c
ifeq ($(CC_ARM),)
	@echo "-- skipping $@ (no aarch64 compiler on $(UNAME_M) host)"
else
	$(CC_ARM) $(CFLAGS) $(LDFLAGS) -o $@ $<
endif

# ---------------------------------------------------------------------------
# The "it works" gate. No -e: we tally failures and report them all at once.
test: all
	@set -uo pipefail; fail=0; \
	echo "########## 1. native codegen computes the predicted values ##########"; \
	./triangle; rc=$$?; \
	if [ "$$rc" = "186" ]; then echo "ok: triangle(100)=5050 -> exit $$rc"; \
	  else echo "FAIL: triangle exit $$rc, expected 186"; fail=1; fi; \
	./args8; rc=$$?; \
	if [ "$$rc" = "121" ]; then echo "ok: pack(1..8)=1401 -> exit $$rc"; \
	  else echo "FAIL: args8 exit $$rc, expected 121"; fail=1; fi; \
	echo; echo "########## 2. aslr_map prints every major region base ##########"; \
	./aslr_map | tee aslr.log; \
	for t in "PIE" "libc text" "loader text" "heap" "stack"; do \
	  grep -q "$$t" aslr.log || { echo "FAIL: aslr_map missing '$$t' region"; fail=1; }; \
	done; \
	n=$$(grep -c "base =" aslr.log || true); \
	[ "$$n" -ge 5 ] || { echo "FAIL: expected >=5 region bases, got $$n"; fail=1; }; \
	echo; echo "########## 3. setarch -R freezes the map (ASLR-off determinism) ##########"; \
	if setarch -R /bin/true >/dev/null 2>&1; then \
	  setarch -R ./aslr_map >a1.txt 2>&1 || true; \
	  setarch -R ./aslr_map >a2.txt 2>&1 || true; \
	  if diff -q a1.txt a2.txt >/dev/null 2>&1; then \
	    echo "ok: two setarch -R runs are byte-identical (ASLR disabled)"; \
	  else echo "FAIL: setarch -R maps differ across runs"; fail=1; fi; \
	else echo "-- setarch -R unavailable here; skipping determinism check"; fi; \
	echo; echo "########## 4. args8 'pack': x86-64 spills to stack, AArch64 does not ##########"; \
	gcc -O1 -c args8.c -o args8.o; \
	./disasm.sh args8.o pack | tee pack.native.txt; \
	case "$(UNAME_M)" in \
	  x86_64) grep -Eq '\[rsp|\[rbp' pack.native.txt \
	            && echo "ok: x86-64 pack reads the 7th/8th args off the stack" \
	            || { echo "FAIL: x86-64 pack shows no stack arg load"; fail=1; } ;; \
	  aarch64|arm64) if grep -q '\[sp' pack.native.txt; then \
	            echo "FAIL: AArch64 pack unexpectedly touched the stack"; fail=1; \
	          else echo "ok: AArch64 pack reads all 8 args from x0-x7 (no stack)"; fi ;; \
	esac; \
	if [ -n "$(CC_ARM)" ] && [ "$(UNAME_M)" = "x86_64" ]; then \
	  $(CC_ARM) -O1 -c args8.c -o args8-arm.o; \
	  ./disasm.sh args8-arm.o pack | tee pack.arm.txt; \
	  if grep -q '\[sp' pack.arm.txt; then \
	    echo "FAIL: AArch64 pack spilled args to the stack"; fail=1; \
	  else echo "ok: cross AArch64 pack uses only x0-x7 (Exercise 2 confirmed)"; fi; \
	fi; \
	echo; echo "########## 5. compare_arch.sh side-by-side (x86-64 host only) ##########"; \
	if [ "$(UNAME_M)" = "x86_64" ] && command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
	  ./compare_arch.sh triangle.c triangle | tee cmp.txt; \
	  grep -q 'System V' cmp.txt && grep -q 'AAPCS64' cmp.txt \
	    || { echo "FAIL: compare_arch.sh headers missing"; fail=1; }; \
	  grep -Eq '\<rax\>|\<rdi\>' cmp.txt || { echo "FAIL: no x86-64 registers in output"; fail=1; }; \
	  grep -Eq '\<x0\>' cmp.txt || { echo "FAIL: no AArch64 registers in output"; fail=1; }; \
	  echo "ok: rendered triangle for x86-64 and AArch64 side by side"; \
	else echo "-- needs x86-64 host + aarch64 cross gcc; skipping side-by-side"; fi; \
	echo; echo "########## 6. optional: run the cross-built arm twins (exit codes) ##########"; \
	if [ -n "$(CC_ARM)" ] && { [ "$(UNAME_M)" != "x86_64" ] || [ -n "$(QEMU_ARM)" ]; }; then \
	  $(MAKE) --no-print-directory arm >/dev/null; \
	  if [ -f triangle.arm ]; then $(RUN_ARM) ./triangle.arm; rc=$$?; \
	    [ "$$rc" = "186" ] && echo "ok: triangle.arm -> exit $$rc" \
	      || { echo "FAIL: triangle.arm exit $$rc"; fail=1; }; fi; \
	  if [ -f args8.arm ]; then $(RUN_ARM) ./args8.arm; rc=$$?; \
	    [ "$$rc" = "121" ] && echo "ok: args8.arm -> exit $$rc" \
	      || { echo "FAIL: args8.arm exit $$rc"; fail=1; }; fi; \
	else echo "-- no aarch64 runner available; skipping arm execution"; fi; \
	echo; echo "########## 7. dynamic linking sub-lab (gotplt/) ##########"; \
	$(MAKE) --no-print-directory -C gotplt test || fail=1; \
	echo; \
	if [ $$fail -ne 0 ]; then echo "ch03-arch: make test FAILED"; exit 1; fi; \
	echo "ch03-arch: all checks passed — make test OK"

clean:
	rm -f $(NATIVE) $(ARMBIN) *.o *.log a1.txt a2.txt pack.native.txt pack.arm.txt cmp.txt
	$(MAKE) --no-print-directory -C gotplt clean
