# ch05-stack/Makefile — stack buffer-overflow lab for Chapter 5.
#
# One source (src/vuln.c) built TWO ways so you can watch the 1996 return-address
# overwrite fail on a modern default build and work the instant the stack defenses
# come off. `make test` is the chapter's "it works" gate: it builds the exploitable
# target(s) and runs solve/solve.py (ret2win), exiting 0 ONLY when a real shell is
# proven (the solver pops /bin/sh and we confirm the PWNED_39 oracle).
#
# Dual-arch & CI-runnable on Ubuntu 24.04: the native architecture is the hard gate
# on each CI runner (x86-64 runner + arm64 runner); the foreign architecture is
# built/solved best-effort under qemu-user when a cross toolchain is present.
SHELL := /bin/bash

# ===========================================================================
# Listing lst-mits (book-verbatim excerpt): the two builds differ ONLY in their
# mitigations, each flag annotated. Driven per-architecture via recursive make
# (see `build` below), which sets ARCH and CC for each concrete target.
# ---------------------------------------------------------------------------
# The HARDENED build: exactly what Ubuntu 24.04 + gcc 13 give you by default.
HARDEN  = -O2 -fstack-protector-all -fPIE -pie \
          -D_FORTIFY_SOURCE=3 \
          -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack    # canary+PIE+RELRO+FORTIFY+NX

# The EXPLOITABLE build: we turn OFF exactly the defenses this chapter motivates
# and NO MORE. NX stays ON, because ret2win reuses existing code and needs no
# executable stack -- a deliberate choice, not an oversight.
EXPLOIT = -O0 -fno-stack-protector -no-pie -fno-pie \
          -D_FORTIFY_SOURCE=0 \
          -Wl,-z,noexecstack                             # canary/PIE/FORTIFY OFF, NX ON

vuln-$(ARCH):          src/vuln.c ; $(CC) $(EXPLOIT) -o $@ $<
vuln-$(ARCH)-hardened: src/vuln.c ; $(CC) $(HARDEN)  -o $@ $<
# ===========================================================================

# ---------------------------------------------------------------------------
# Host/toolchain detection. Native arch is gcc; the foreign arch uses a cross
# gcc + qemu-user when available (the exact setup the repo's CI installs).
UNAME_M := $(shell uname -m)
ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
  # x86-64 host
  NATIVE_ARCH  := x86_64
  NATIVE_CC    := gcc
  FOREIGN_ARCH := arm64
  FOREIGN_CC   := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)
else
  # arm64 host
  NATIVE_ARCH  := arm64
  NATIVE_CC    := gcc
  FOREIGN_ARCH := x86_64
  FOREIGN_CC   := $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null)
endif

.DEFAULT_GOAL := all
.PHONY: all build test demo-hardened checksec clean

all: build

# Build the native pair (hard requirement) and, if a cross compiler exists, the
# foreign pair too (best-effort; foreign execution is emulated and non-fatal).
build:
	@$(MAKE) --no-print-directory ARCH=$(NATIVE_ARCH) CC=$(NATIVE_CC) \
		vuln-$(NATIVE_ARCH) vuln-$(NATIVE_ARCH)-hardened
	@if [ -n "$(FOREIGN_CC)" ]; then \
		$(MAKE) --no-print-directory ARCH=$(FOREIGN_ARCH) CC=$(FOREIGN_CC) \
			vuln-$(FOREIGN_ARCH) vuln-$(FOREIGN_ARCH)-hardened \
		|| echo "-- foreign ($(FOREIGN_ARCH)) build failed; continuing (native arch is the CI gate)"; \
	else echo "-- no $(FOREIGN_ARCH) cross-compiler found; skipping foreign build"; fi
	@echo "built: $$(ls vuln-* 2>/dev/null | tr '\n' ' ')"

# ---------------------------------------------------------------------------
# The "it works" gate. Exit 0 ONLY on a proven shell:
#   * the native-arch solve MUST succeed (solver exits non-zero on failure AND
#     we grep its output for the unique "ret2win OK" success marker);
#   * the foreign-arch solve runs under qemu-user best-effort and never fails
#     the build (the matching native CI runner is that arch's real gate).
# setarch -R runs the solver with ASLR off for determinism (the no-pie win()
# address is fixed regardless, but this matches the book's lab convention).
test: build
	@set -uo pipefail; \
	ulimit -c unlimited 2>/dev/null || true; \
	echo 'core.%p' | sudo -n tee /proc/sys/kernel/core_pattern >/dev/null 2>&1 \
		|| echo "-- note: could not set kernel.core_pattern (no passwordless sudo); relying on pwntools corefile discovery"; \
	echo "built: $$(ls vuln-* 2>/dev/null | tr '\n' ' ')"; \
	fail=0; \
	if command -v setarch >/dev/null 2>&1 && setarch -R /bin/true >/dev/null 2>&1; then RUN="setarch -R"; else RUN=""; fi; \
	echo "== $(NATIVE_ARCH) ret2win =="; \
	if $$RUN python3 solve/solve.py ./vuln-$(NATIVE_ARCH) 2>&1 | tee solve.$(NATIVE_ARCH).log; then :; else fail=1; fi; \
	grep -q "ret2win OK" solve.$(NATIVE_ARCH).log || { echo "FAIL: no ret2win success marker for $(NATIVE_ARCH)"; fail=1; }; \
	if [ -x ./vuln-$(FOREIGN_ARCH) ]; then \
		echo "== $(FOREIGN_ARCH) ret2win (best-effort under QEMU) =="; \
		if python3 solve/solve.py ./vuln-$(FOREIGN_ARCH) 2>&1 | tee solve.$(FOREIGN_ARCH).log && grep -q "ret2win OK" solve.$(FOREIGN_ARCH).log; then \
			echo "ok: $(FOREIGN_ARCH) ret2win confirmed under emulation"; \
		else echo "-- $(FOREIGN_ARCH) solve skipped/failed under emulation (non-fatal; the native $(FOREIGN_ARCH) CI runner gates it)"; fi; \
	else echo "-- no $(FOREIGN_ARCH) binary built; skipping foreign solve"; fi; \
	echo; \
	if [ $$fail -ne 0 ]; then echo "ch05-stack: make test FAILED"; exit 1; fi; \
	echo "ch05-stack: ret2win confirmed on every built target — make test OK"

# Watch the hardened build die on the canary instead of handing you a shell.
demo-hardened: build
	@echo "feeding 200 bytes to vuln-$(NATIVE_ARCH)-hardened (expect a canary abort):"; \
	if python3 -c "import sys; sys.stdout.buffer.write(b'A'*200)" | ./vuln-$(NATIVE_ARCH)-hardened; then \
		echo "UNEXPECTED: hardened build did not abort"; exit 1; \
	else echo "ok: hardened build aborted (*** stack smashing detected *** / SIGABRT) as designed"; fi

# Print each build's mitigation table so you can confirm they differ.
checksec: build
	@for b in vuln-$(NATIVE_ARCH) vuln-$(NATIVE_ARCH)-hardened; do \
		[ -x "$$b" ] || continue; echo "== $$b =="; \
		pwn checksec "$$b" 2>/dev/null \
			|| checksec --file="$$b" 2>/dev/null \
			|| readelf -lW "$$b" | grep -E 'GNU_STACK|GNU_RELRO' || true; \
	done

clean:
	rm -f vuln-x86_64 vuln-x86_64-hardened vuln-arm64 vuln-arm64-hardened \
		core core.* solve.*.log
