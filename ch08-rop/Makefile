# ch08-rop/Makefile - build the lab targets and prove the exploits.
#
#   make              build every target this host can build
#   make test         build + run the exploits; exit 0 ONLY on success
#   make test-x86_64  just the x86-64 ret2libc + pivot gates
#   make test-arm64   just the AArch64 ret2libc lane
#   make clean
#
# CI runs `make test`. The x86-64 ret2libc and the stack-pivot demo are HARD
# gates (they must pop a shell, asserted by grepping a unique PWNED marker).
# The AArch64 lane is a hard gate on a NATIVE aarch64 host and best-effort
# under qemu (see exploit/solve_arm64.py for why). Determinism: the x86-64
# runs are pinned with `setarch -R` so stack alignment is fixed.

HOST          := $(shell uname -m)
X86_BUILDABLE := $(shell gcc -dumpmachine 2>/dev/null | grep -q x86_64 && echo yes)

.PHONY: all test test-x86_64 test-arm64 clean

all:
	$(MAKE) -C target all

# --- aggregate test ---------------------------------------------------------
test:
	@bash ci-test.sh

# --- individual lanes (also callable directly) ------------------------------
test-x86_64:
	@bash ci-test.sh x86_64

test-arm64:
	@bash ci-test.sh arm64

clean:
	$(MAKE) -C target clean
	rm -f exploit/vuln-x86_64 exploit/vuln_pivot-x86_64 exploit/vuln-arm64
	rm -f exploit/core* target/core* core*
