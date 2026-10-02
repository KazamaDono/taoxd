# ch10-aslr/leak-server — leak + ret2libc lab.
#
# Mitigation flags (documented in every chapter-10 target):
#
#   -fno-stack-protector : canary OFF, so the linear overflow reaches the
#                          saved return address without smashing a cookie.
#                          Canaries are the subject of the NEXT chapter; we
#                          intentionally isolate one defense at a time.
#   -no-pie              : PIE OFF, so vuln's own .text sits at 0x400000.
#                          Simplifies the ROP because we know executable
#                          gadget addresses without a second leak.  ASLR
#                          for shared libraries is UNAFFECTED by this
#                          option: libc still gets a random base, which is
#                          the entire point of the leak-then-chain flow.
#   -z noexecstack       : NX ON (default on Ubuntu 24.04).  Explicit so
#                          the intent is visible when reading the build.
#   -D_GNU_SOURCE        : exposes RTLD_DEFAULT for the dlsym-based leak.
#   -U_FORTIFY_SOURCE \
#   -D_FORTIFY_SOURCE=0  : FORTIFY OFF.  The pinned lab image's gcc enables
#                          _FORTIFY_SOURCE=3 by default at -O1, which rewrites
#                          `read(0, name, 512)` into `__read_chk(.., 512, 64)`.
#                          Because the 512-byte request exceeds the 64-byte
#                          object size, __chk_fail() aborts the process
#                          (SIGABRT) *before* the overflow can reach the saved
#                          return address -- the intended vulnerability never
#                          fires.  We must disable FORTIFY explicitly so the
#                          teaching overflow is genuine; it is a buffer-size
#                          check (Ch12 material), not the Ch10 subject.
#   -O1 -fno-inline      : keep `banner`, `handle`, `main` as separate
#                          real function frames so the buffer→RIP offset
#                          in the solver (72) matches the layout.
#   -Wl,-z,relro,-z,now  : Full RELRO ON.  Doesn't matter here (we don't
#                          write to the GOT), but keeps the build modern.
#
# ASLR at the *system* level is left ON; the solver defeats it via the
# leak the target publishes.  The `test` target runs the exploit against
# a fresh process each time -- fresh execve, fresh libc base.

CC       ?= gcc
CFLAGS   ?= -O1 -Wall -Wno-format-security -Wno-stringop-overflow -fno-inline \
            -fno-stack-protector -no-pie -D_GNU_SOURCE \
            -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
LDFLAGS  ?= -Wl,-z,relro,-z,now -Wl,-z,noexecstack

AARCH64_CC ?= aarch64-linux-gnu-gcc
AARCH64_CFLAGS ?= -O1 -Wall -Wno-format-security -Wno-stringop-overflow \
                  -fno-inline -fno-stack-protector -D_GNU_SOURCE \
                  -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

PYTHON ?= python3

.PHONY: all test clean arm64 arm64-maybe check-tools

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# AArch64 cross build.  Optional -- only invoked when the cross toolchain
# is installed.  CI on ubuntu-latest amd64 does NOT gate on this.
arm64: vuln-arm64
vuln-arm64: vuln-arm64.c
	$(AARCH64_CC) $(AARCH64_CFLAGS) -o $@ $<

arm64-maybe:
	@if command -v $(AARCH64_CC) >/dev/null 2>&1; then \
	    $(MAKE) arm64; \
	else \
	    echo "[skip] $(AARCH64_CC) not installed; skipping arm64 build."; \
	fi

check-tools:
	@$(PYTHON) -c "import pwn, pytest" 2>/dev/null || { \
	    echo "[test] installing pwntools + pytest into user site..."; \
	    $(PYTHON) -m pip install --quiet --user \
	        "pwntools>=4.12,<5" "pytest>=7,<9" \
	        || { echo "[test] pip install failed"; exit 1; }; \
	}

# The CI gate.  Builds the target, then runs the pytest harness which
# executes the solver N times against fresh processes (fresh libc bases)
# and asserts a shell responds.  Exits 0 only when every run popped.
test: vuln check-tools
	CH10_RUNS=$${CH10_RUNS:-5} $(PYTHON) -m pytest -q tests/test_solve.py

clean:
	rm -f vuln vuln-arm64
