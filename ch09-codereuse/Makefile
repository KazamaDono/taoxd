# ch09-codereuse/Makefile — Advanced Code Reuse (JOP/COP, SROP, ret2dlresolve,
# ret2csu) lab for Chapter 9. CI-runnable on Ubuntu 24.04 (glibc 2.39, gcc 13),
# on both the x86-64 and the arm64 runner.
#
# `make test` is the "it works" gate. It is deterministic and EXITS 0 ONLY ON
# SUCCESS: each exploit runs under a timeout and must print a unique marker
# (SROP_OK / DLR_OK) *and* a real `id` line (uid=...) from a spawned shell.
#
# ---------------------------------------------------------------------------
# Mitigation flags are EXPLICIT and COMMENTED per target (which defense is on
# or off, and why). These binaries are deliberately vulnerable teaching
# artifacts — see ../ETHICS.md.
#
#   SROP target (srop/vuln):
#     -fno-stack-protector  CANARY OFF  — the linear read() overflow must be
#                                         able to reach and overwrite the saved
#                                         return address.
#     -no-pie -fno-pie      PIE  OFF    — the planted syscall;ret / pop rax;ret
#                                         gadgets and the "/bin/sh" string sit
#                                         at FIXED addresses, so the leak-free
#                                         solve can name them directly.
#     (NX stays ON — the distro default noexecstack. SROP never executes stack
#      memory, so NX is irrelevant here and we leave the modern default in.)
#
#   ret2dlresolve target (dlresolve/vuln):
#     -fno-stack-protector  CANARY OFF  — same reason: reach the return address.
#     -no-pie -fno-pie      PIE  OFF    — fixed .bss staging + PLT[0] address,
#                                         no leak required.
#     -Wl,-z,relro          PARTIAL RELRO — .got is read-only but .got.plt (the
#                                         lazy PLT GOT) stays WRITABLE.
#     -Wl,-z,lazy           LAZY BINDING — PLT[0] is a live _dl_runtime_resolve
#                                         trampoline. THIS IS REQUIRED: Full
#                                         RELRO / -z now resolves every import
#                                         at startup, marks the whole GOT RO,
#                                         and removes the lazy path — which kills
#                                         ret2dlresolve. (Chapter 9, Exercise 3.)
#     (NX stays ON.)
# ---------------------------------------------------------------------------
SHELL := /bin/bash
PY    := python3

CC           := gcc
CFLAGS_SROP  := -O0 -g -fno-stack-protector -no-pie -fno-pie
CFLAGS_DLR   := -O0 -g -fno-stack-protector -no-pie -fno-pie
LDFLAGS_DLR  := -no-pie -Wl,-z,relro -Wl,-z,lazy

# AArch64 cross build for the SROP port (run under qemu-user on an x86-64 host,
# native on an arm64 runner). Static so qemu-user needs no aarch64 sysroot.
UNAME_M := $(shell uname -m)
ifeq ($(filter aarch64 arm64,$(UNAME_M)),)
  CC_ARM   := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)
  QEMU_ARM := $(shell command -v qemu-aarch64 2>/dev/null)
else
  CC_ARM   := gcc
  QEMU_ARM := native
endif
CFLAGS_ARM := -O0 -g -fno-stack-protector -no-pie -static

.DEFAULT_GOAL := all
.PHONY: all arm test test-srop test-dlresolve test-csu test-jop test-arm64 clean

all: srop/vuln dlresolve/vuln
	@chmod +x jop/find_jop.sh csu/csu_probe.sh
	@echo "built: srop/vuln dlresolve/vuln"

srop/vuln: srop/vuln.c
	$(CC) $(CFLAGS_SROP) -o $@ $<

dlresolve/vuln: dlresolve/vuln.c
	$(CC) $(CFLAGS_DLR) $(LDFLAGS_DLR) -o $@ $<

# AArch64 SROP twin (best effort). Skipped cleanly if no cross compiler.
arm: srop/vuln_arm64
srop/vuln_arm64: srop/vuln_arm64.c
ifeq ($(CC_ARM),)
	@echo "-- skipping srop/vuln_arm64 (no aarch64 compiler on $(UNAME_M) host)"
else
	$(CC_ARM) $(CFLAGS_ARM) -o $@ $<
endif

# ---------------------------------------------------------------------------
# The gate. No -e at top level: tally failures and report them all at once.
test: all
	@set -uo pipefail; fail=0; \
	$(MAKE) --no-print-directory test-srop      || fail=1; \
	echo; $(MAKE) --no-print-directory test-dlresolve || fail=1; \
	echo; $(MAKE) --no-print-directory test-csu  || fail=1; \
	echo; $(MAKE) --no-print-directory test-jop  || fail=1; \
	echo; $(MAKE) --no-print-directory test-arm64 || fail=1; \
	echo; \
	if [ $$fail -ne 0 ]; then echo "ch09-codereuse: make test FAILED"; exit 1; fi; \
	echo "ch09-codereuse: all checks passed — make test OK"

# 1. SROP: forge a signal frame -> execve("/bin/sh"). ASLR is irrelevant (the
#    binary is -no-pie and we use no stack addresses), but run under setarch -R
#    when available for fully frozen, reproducible behavior.
test-srop: srop/vuln
	@set -uo pipefail; \
	echo "########## 1. SROP: one forged frame -> shell ##########"; \
	runner=""; setarch -R /bin/true >/dev/null 2>&1 && runner="setarch -R"; \
	out=$$(cd srop && timeout 60 $$runner $(PY) solve.py 2>&1); rc=$$?; \
	echo "$$out"; \
	if [ $$rc -eq 0 ] && echo "$$out" | grep -q "SROP_OK" && echo "$$out" | grep -q "uid="; then \
	  echo "ok: SROP forged-frame execve spawned a shell"; \
	else echo "FAIL: SROP solve did not reach a shell (rc=$$rc)"; exit 1; fi

# 2. ret2dlresolve: resolve+call system("/bin/sh") with NO libc leak.
test-dlresolve: dlresolve/vuln
	@set -uo pipefail; \
	echo "########## 2. ret2dlresolve: system(\"/bin/sh\"), no leak ##########"; \
	runner=""; setarch -R /bin/true >/dev/null 2>&1 && runner="setarch -R"; \
	out=$$(cd dlresolve && timeout 60 $$runner $(PY) solve.py 2>&1); rc=$$?; \
	echo "$$out"; \
	if [ $$rc -eq 0 ] && echo "$$out" | grep -q "DLR_OK" && echo "$$out" | grep -q "uid="; then \
	  echo "ok: ret2dlresolve called system without a leak"; \
	else echo "FAIL: ret2dlresolve solve did not reach a shell (rc=$$rc)"; exit 1; fi

# 3. csu_probe: on glibc 2.39 the universal gadget is GONE. The probe must say
#    so (the modern, expected outcome on Ubuntu 24.04).
test-csu: srop/vuln dlresolve/vuln
	@set -uo pipefail; \
	echo "########## 3. csu_probe: __libc_csu_init absent on glibc 2.39 ##########"; \
	out=$$(bash csu/csu_probe.sh dlresolve/vuln 2>&1); \
	echo "$$out"; \
	if echo "$$out" | grep -q "no __libc_csu_init"; then \
	  echo "ok: probe correctly reports the universal gadget is gone (>= glibc 2.34)"; \
	else echo "FAIL: csu_probe did not report the glibc 2.34 removal"; exit 1; fi

# 4. find_jop: must run and emit its section headers against a real target.
#    (We assert it executes and produces its report; gadget counts vary by
#    binary and tool version, so we don't gate on a specific count.)
test-jop: srop/vuln
	@set -uo pipefail; \
	echo "########## 4. find_jop: enumerate indirect-branch gadgets ##########"; \
	out=$$(bash jop/find_jop.sh srop/vuln 2>&1 || true); \
	echo "$$out" | head -20; \
	if echo "$$out" | grep -q "dispatcher candidates"; then \
	  echo "ok: find_jop.sh ran and produced its JOP/COP report"; \
	else echo "FAIL: find_jop.sh did not run (missing ropper/ROPgadget?)"; exit 1; fi

# 5. AArch64 SROP under qemu-user (best effort, NON-FATAL). Ships and runs when
#    the cross toolchain + qemu-user are present; skipped cleanly otherwise so
#    it never breaks the x86-64 CI gate.
test-arm64:
	@set -uo pipefail; \
	echo "########## 5. AArch64 SROP under qemu-user (best effort) ##########"; \
	if [ -z "$(CC_ARM)" ]; then echo "-- no aarch64 compiler; skipping arm64 SROP"; exit 0; fi; \
	if [ "$(QEMU_ARM)" != "native" ] && [ -z "$(QEMU_ARM)" ]; then \
	  echo "-- no qemu-aarch64; skipping arm64 SROP"; exit 0; fi; \
	$(MAKE) --no-print-directory arm >/dev/null; \
	if [ ! -f srop/vuln_arm64 ]; then echo "-- arm64 target not built; skipping"; exit 0; fi; \
	out=$$(cd srop && timeout 120 $(PY) solve_arm64.py 2>&1); rc=$$?; \
	echo "$$out"; \
	if [ $$rc -eq 0 ] && echo "$$out" | grep -q "uid="; then \
	  echo "ok: AArch64 SROP spawned a shell under qemu-user"; \
	else echo "-- arm64 SROP did not confirm (non-fatal; x86-64 gate is authoritative)"; fi

clean:
	rm -f srop/vuln srop/vuln_arm64 dlresolve/vuln
	rm -rf srop/__pycache__ dlresolve/__pycache__
