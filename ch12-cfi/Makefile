# ch12-cfi/Makefile -- top-level for the chapter.  Dispatches to the runnable
# sub-lab (cfi-icall) and runs a self-test of the kCFI probe.  The xfg/
# subdirectory is documentation-only (Windows/MSVC), so it is not built here.

.PHONY: all test clean

all:
	$(MAKE) -C cfi-icall all

# `make test` is what CI invokes.  It:
#   1. builds and tests the Clang cfi-icall demos (demo + reuse), and
#   2. runs the kCFI probe against a synthesized self-test ELF, so the
#      pattern-matcher is exercised on every push even when no kernel
#      module is available.
test:
	$(MAKE) -C cfi-icall test
	@echo
	@echo "=== kCFI probe self-test ==="
	@bash kcfi/kcfi-probe.sh --self-test \
		|| { echo "FAIL: kcfi-probe self-test"; exit 1; }
	@echo
	@echo "ch12-cfi: PASS"

clean:
	$(MAKE) -C cfi-icall clean
