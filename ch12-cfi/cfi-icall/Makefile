# ch12-cfi/cfi-icall/Makefile
# Build the two Clang CFI teaching binaries plus a non-CFI comparison build.
# Ubuntu 24.04 / clang 18 / lld 18.
#
# Mitigation flag map:
#   -O1                     : optimizer on (LTO needs at least -O1 to be useful)
#   -flto                   : LINK-TIME OPTIMIZATION -- required for CFI so the
#                             linker can compute the whole-program type set.
#   -fvisibility=hidden     : makes address-taken functions internal, tightens
#                             the CFI equivalence class (matches book listing).
#   -fsanitize=cfi          : enable ALL Clang forward-edge CFI checks
#                             (cfi-icall + cfi-vcall + cfi-nvcall + ...).
#   -fno-sanitize-trap=cfi  : on failure, print a human-readable diagnostic
#                             instead of executing ud2.  Book demo uses this so
#                             the runtime prints the "control flow integrity
#                             check for type ..." line.
#   -fsanitize-recover=cfi  : after printing the diagnostic, KEEP GOING rather
#                             than abort.  Lets the demo show that a production
#                             build with -fsanitize-trap=cfi would have SIGILL'd.
#   -fuse-ld=lld            : lld is the LTO-aware linker on Ubuntu 24.04.
#
# The `reuse` binary uses -fsanitize-trap=cfi (the production default) to
# demonstrate that a same-signature swap does NOT trap -- the check passes.
#
# The `demo-nocfi` binary is the same source built WITHOUT CFI, to show the
# mistyped call runs (and typically crashes deep inside puts) when the defense
# is absent.

CLANG    ?= clang-18
CFI_CFLAGS := -O1 -flto -fvisibility=hidden -fsanitize=cfi -fuse-ld=lld
NOCFI_CFLAGS := -O1 -fvisibility=hidden

.PHONY: all clean test deps

all: demo reuse demo-nocfi

# Demo built with recovering CFI so the diagnostic is printed to stderr and
# execution continues; the test greps stderr for the CFI marker.
demo: demo.c
	$(CLANG) $(CFI_CFLAGS) -fno-sanitize-trap=cfi -fsanitize-recover=cfi \
		-o $@ $<

# Reuse built with the production default (trap on failure).  We EXPECT the
# same-signature swap to pass silently -- the test asserts the admin banner
# printed and the process exited 0.
reuse: reuse.c
	$(CLANG) $(CFI_CFLAGS) -fsanitize-trap=cfi -o $@ $<

# Same source, no CFI, for the "what would have happened" comparison.
demo-nocfi: demo.c
	$(CLANG) $(NOCFI_CFLAGS) -o $@ $<

# Best-effort install of clang-18/lld-18 if the lab image lacks them.
deps:
	@command -v $(CLANG) >/dev/null 2>&1 || { \
		echo "installing clang-18/lld-18..."; \
		sudo apt-get update -qq && \
		sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
			clang-18 lld-18 llvm-18 libclang-rt-18-dev ; \
	}
	@# libclang-rt-18-dev supplies the UBSan/CFI runtime archives that
	@# -fsanitize=cfi + -fno-sanitize-trap need for the diagnostic printer.
	@dpkg -s libclang-rt-18-dev >/dev/null 2>&1 || \
		sudo DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
			libclang-rt-18-dev || true

# `make test` is the CI gate.  It exits 0 only if:
#   (1) demo prints the Clang CFI diagnostic (proves cfi-icall rejected the
#       wrongly-typed indirect call), AND
#   (2) reuse prints the admin banner and exits 0 (proves same-signature
#       reuse slips through -- the point of Listing 12.2).
test: deps all
	@echo "=== [1/2] demo: CFI must REJECT the mistyped call ==="
	@./demo > demo.out 2> demo.err || true
	@cat demo.out demo.err
	@grep -qE 'control flow integrity|cfi-icall|runtime error' demo.err \
		|| { echo "FAIL: expected CFI diagnostic on stderr"; exit 1; }
	@echo "OK: cfi-icall diagnostic observed"
	@echo
	@echo "=== [2/2] reuse: CFI must ACCEPT the same-signature swap ==="
	@./reuse > reuse.out 2> reuse.err
	@cat reuse.out reuse.err
	@grep -q 'admin_shell reached' reuse.out \
		|| { echo "FAIL: expected admin_shell banner"; exit 1; }
	@echo "OK: same-type reuse passed CFI (as designed)"
	@echo
	@echo "ch12-cfi/cfi-icall: PASS"

clean:
	rm -f demo reuse demo-nocfi *.out *.err
