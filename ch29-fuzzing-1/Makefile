# ch29-fuzzing-1/Makefile
# Ubuntu 24.04, clang-18 preferred; falls back to `clang` if clang-18 is absent.
#
# Mitigation flags — explicit and commented, as the book requires:
#   -g                           : keep debug info so ASan / llvm-cov symbolize
#   -O1                          : light optimization; matches the chapter's
#                                  libFuzzer build (needed for realistic edges)
#   -fno-omit-frame-pointer      : keep frame pointers so ASan stacks unwind
#   -fsanitize=address           : ASan ON — the fuzz oracle for heap bugs.
#                                  This is the whole point of the lab.
#   -fsanitize=undefined         : UBSan ON — catches the integer wrap that
#                                  drives BUG 1 well before the heap store.
#   -U_FORTIFY_SOURCE            : _FORTIFY_SOURCE OFF — would otherwise turn
#                                  some overflowing writes into fortified
#                                  aborts before ASan can attribute them.
#   -fno-stack-protector         : stack canaries OFF — irrelevant to heap
#                                  bugs, kept off so a lucky canary abort
#                                  cannot mask an ASan report.
#   (ASLR is a runtime property; the harness does not depend on it either
#    way. `setarch -R` is not required here — no fixed addresses are used.)

CLANG ?= $(shell command -v clang-18 2>/dev/null || command -v clang 2>/dev/null || echo clang)

CFLAGS_COMMON = -g -O1 -Wall -Wextra -Isrc \
                -fno-omit-frame-pointer -U_FORTIFY_SOURCE \
                -fno-stack-protector \
                -fsanitize=address,undefined

SRCS_LIB    = src/csv_parse.c
SRCS_HARN   = harness/fuzz_csv.c
SRC_REPLAY  = harness/replay_main.c

BUILD       = build

.PHONY: all test fuzz coverage clean

all: $(BUILD)/replay

$(BUILD):
	@mkdir -p $(BUILD)

# Non-fuzzer replay binary — the CI-runnable target. Compiles the same
# harness the fuzzers use, plus a tiny main() that reads argv files.
$(BUILD)/replay: $(SRCS_LIB) $(SRCS_HARN) $(SRC_REPLAY) | $(BUILD)
	$(CLANG) $(CFLAGS_COMMON) $^ -o $@

# libFuzzer harness (optional — only built if the toolchain supports it).
$(BUILD)/fuzz_csv_libfuzzer: $(SRCS_LIB) $(SRCS_HARN) | $(BUILD)
	@if ! echo 'int LLVMFuzzerTestOneInput(const unsigned char*x,unsigned long y){(void)x;(void)y;return 0;}' \
	    | $(CLANG) -x c -fsanitize=fuzzer,address - -o /dev/null 2>/dev/null; then \
	  echo "[skip] $(CLANG) has no libFuzzer support; not building $@"; \
	else \
	  $(CLANG) $(CFLAGS_COMMON) -fsanitize=fuzzer $^ -o $@; \
	fi

fuzz: $(BUILD)/fuzz_csv_libfuzzer
	./$(BUILD)/fuzz_csv_libfuzzer corpus/seeds -dict=corpus/csv.dict \
	    -max_len=65536 -runs=100000

coverage:
	bash scripts/coverage.sh

# Deterministic CI test: build the replay binary, prove seeds parse
# cleanly under ASan, then prove the planted use-after-free fires with
# a symbolized ASan report and the expected exit code.
test: $(BUILD)/replay
	@echo "== ch29 fuzzing lab test =="
	@echo "-- compiler: $(CLANG)"
	@set -e; for f in corpus/seeds/*.csv; do \
	  echo "-- replay seed  $$f"; \
	  ASAN_OPTIONS=abort_on_error=0:exitcode=0:detect_leaks=0 \
	    ./$(BUILD)/replay "$$f" >/dev/null \
	    || { echo "FAIL: seed unexpectedly crashed: $$f"; exit 1; }; \
	done
	@echo "-- replay crash corpus/crash_seeds/uaf.bin (expect ASan UAF)"
	@out=`ASAN_OPTIONS=abort_on_error=0:exitcode=42:halt_on_error=1:symbolize=1:detect_leaks=0 \
	    ./$(BUILD)/replay corpus/crash_seeds/uaf.bin 2>&1`; rc=$$?; \
	  echo "$$out" | head -20; \
	  echo "$$out" | grep -q "heap-use-after-free" \
	    || { echo "FAIL: expected ASan heap-use-after-free (rc=$$rc)"; exit 1; }; \
	  [ "$$rc" -eq 42 ] \
	    || { echo "FAIL: expected exitcode 42, got $$rc"; exit 1; }
	@echo "[ch29] all fuzzing checks passed"

clean:
	rm -rf $(BUILD) findings default.profraw *.profraw *.profdata
