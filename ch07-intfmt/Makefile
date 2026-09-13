# ch07-intfmt/Makefile -- build the Chapter 7 lab targets and prove the
# exploits. LAB ONLY: these binaries are vulnerable on purpose. `make test`
# builds everything, runs both solve scripts, and exits 0 ONLY if each reaches
# its win marker; it is deterministic (ASLR off via `setarch -R`).
#
# CI runs `make test` on Ubuntu 24.04 (gcc 13, glibc 2.39) on x86-64 and arm64.

CC      ?= gcc
CLANG   ?= clang-18
PY      ?= python3
# Disable ASLR for child processes so the run is reproducible (no privilege
# needed; affects only these processes, never the host). The format-string
# solve discovers its offset at runtime and targets a -no-pie global, so it
# does not strictly depend on this -- we pin it anyway for determinism.
SETARCH := setarch -R

# ---- fmt: deliberately vulnerable format-string target ----------------------
#   -no-pie               : fixed load base, so &authorized is a known constant
#                           -> no leak needed before the %n write.
#   -fno-stack-protector  : no canary (irrelevant to this bug; keeps asm clean).
#   -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 : DISABLE FORTIFY. Ubuntu's gcc turns
#                           on _FORTIFY_SOURCE=3 by default when optimizing,
#                           which rewrites printf -> __printf_chk and refuses
#                           `%n` in a writable format. We switch it OFF so the
#                           classic %n arbitrary write is reachable at all.
#   -O0                   : stable stack layout (offset still found at runtime).
#   -Wno-format-security  : silence the (correct!) printf(buf) warning; the bug
#                           is the entire point of this target.
#   NX/DEP stays ON (default): we never execute the buffer, we flip a flag.
FMT_CFLAGS = -no-pie -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
             -O0 -g -Wno-format-security

# ---- intmul: integer overflow -> undersized malloc -> heap overflow ---------
#   -O1                   : the 32-bit multiply wraps identically at any -O; we
#                           pin -O1 for a stable, deterministic heap layout.
#   FORTIFY is left at Ubuntu's default (ON) ON PURPOSE: it does NOTHING for
#                           this class -- it guards bounds it can prove, not a
#                           wrapped size calc -- which is exercise 4. The bug is
#                           reachable regardless.
#   -fno-stack-protector  : minimal layout; the corruption is on the heap.
INTMUL_CFLAGS = -O1 -g -fno-stack-protector

# ---- intmul_fixed: the remediation (C23 checked multiply) -------------------
#   -std=c2x              : enable C23 so <stdckdint.h>'s ckd_mul is visible on
#                           clang 18 / gcc 14+. On gcc 13 (Ubuntu 24.04 default)
#                           the header does not exist yet, so the source falls
#                           back to __builtin_mul_overflow via __has_include.
FIXED_CFLAGS = -O1 -g -std=c2x

.PHONY: all test sanitizers clean
all: fmt intmul intmul_fixed

fmt: src/fmt.c
	$(CC) $(FMT_CFLAGS) -o $@ $<

intmul: src/intmul.c
	$(CC) $(INTMUL_CFLAGS) -o $@ $<

intmul_fixed: src/intmul_fixed.c
	$(CC) $(FIXED_CFLAGS) -o $@ $<

# Deterministic end-to-end proof. Each step greps a unique success marker and
# aborts non-zero otherwise; `timeout` bounds any accidental hang so CI cannot
# stall. Needs pwntools (present in the lab image and installed by CI).
test: all
	@command -v $(PY) >/dev/null 2>&1 || { echo "FAIL: $(PY) not found"; exit 1; }
	@$(PY) -c 'import pwn' 2>/dev/null || { echo "FAIL: pwntools not installed (pip install pwntools)"; exit 1; }
	@echo "== [1/3] format string: printf(buf) -> %n write -> shell =="
	@$(SETARCH) timeout 120 $(PY) solve/solve_fmt.py BIN=./fmt 2>&1 | tee /tmp/ch07_fmt.log
	@grep -q 'arbitrary write -> code execution' /tmp/ch07_fmt.log \
	  || { echo 'FAIL: fmt exploit did not reach code execution'; exit 1; }
	@echo "== [2/3] integer overflow -> heap overflow -> data-only win =="
	@$(SETARCH) timeout 60 $(PY) solve/solve_intmul.py BIN=./intmul 2>&1 | tee /tmp/ch07_int.log
	@grep -q 'privilege flag set' /tmp/ch07_int.log \
	  || { echo 'FAIL: intmul exploit did not set is_admin'; exit 1; }
	@echo "== [3/3] remediation: ckd_mul rejects the overflowing size =="
	@printf '\x02\x00\x00\x08\x78\x00\x00\x00\x00\x00\x00\x00' > /tmp/ch07_ovf.bin
	@if ./intmul_fixed < /tmp/ch07_ovf.bin 2>&1 | grep -q 'size overflow rejected'; then \
	    echo 'OK: fixed build refused the wrapped size'; \
	  else echo 'FAIL: fixed build did not reject the overflow'; exit 1; fi
	@echo "ALL CH07 TESTS PASSED"

# Optional sanitizer demos from the chapter. The unsigned-integer-overflow and
# implicit-integer-truncation UBSan checks are CLANG features (GCC implements
# neither), so these use clang-18. Best-effort: skipped cleanly if absent. Not
# part of `make test`, so CI (which may lack clang) is unaffected.
sanitizers:
	@command -v $(CLANG) >/dev/null 2>&1 || { echo "skip: $(CLANG) not installed"; exit 0; }
	$(CLANG) -O1 -g -fsanitize=unsigned-integer-overflow,implicit-integer-truncation \
	    -fno-sanitize-recover=all -o intmul.ubsan src/intmul.c
	$(CLANG) -O1 -g -fsanitize=address -o intmul.asan src/intmul.c
	@echo "built intmul.ubsan (UBSan) and intmul.asan (ASan)"
	@echo "drive with the 12-byte overflow header, e.g.:"
	@echo "  printf '\\\\x02\\\\x00\\\\x00\\\\x08\\\\x78\\\\x00\\\\x00\\\\x00\\\\x00\\\\x00\\\\x00\\\\x00' | ./intmul.ubsan"

clean:
	rm -f fmt intmul intmul_fixed intmul.ubsan intmul.asan
