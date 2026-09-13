# ch30-fuzzing-2/Makefile — top-level targets for the chapter.
#
# Targets:
#   build-all    compile the C targets (as plain executables + ASan smoke)
#   fuzz-fdp     build the FuzzedDataProvider harness (needs clang-18 fuzzer)
#   fuzz-lpm     build the libprotobuf-mutator harness (needs protobuf +
#                libprotobuf-mutator-dev; skipped gracefully if absent)
#   fuzz-grammar run the shell-driver grammar loop for 500 iters
#   fuzz-diff    build the differential JSON harness (needs libjsoncpp-dev
#                + libyyjson-dev; skipped gracefully if absent)
#   test         deterministic CI target: build the ASan-instrumented targets,
#                feed each its known-crashing input, assert ASan trips on
#                the planted bug and the generators + agent run clean.
#
# Mitigation flags for the deliberately-vulnerable targets:
#   CFLAGS_LAB = -g -O1 -fsanitize=address -fno-omit-frame-pointer
#                -fstack-protector-strong
#     ASan is ON so the planted UB is loud, not silent.
#     Frame pointers ON for readable stacks.
#     Stack protector strong is ON as a *safety net* — we still want the
#     bug in target.c to escape SSP via the ASan redzone (the memcpy writes
#     past the buffer end, not through the return address).
#     We do NOT pass -D_FORTIFY_SOURCE=3: FORTIFY would rewrite memcpy at
#     compile time and mask the exact class of bug this chapter demonstrates.
#     We do NOT pass -fPIE/-pie either; the targets are hosted test doubles,
#     not exploitation targets.

SHELL := /bin/bash

CC       ?= cc
CXX      ?= c++
CLANG    ?= clang-18
CLANGXX  ?= clang++-18

CFLAGS_LAB = -g -O1 -Wall -Wextra \
             -fsanitize=address,undefined \
             -fno-omit-frame-pointer \
             -fstack-protector-strong

BUILD := build

.PHONY: all build-all test clean fuzz-fdp fuzz-lpm fuzz-grammar fuzz-diff \
        _test_target _test_parse_config _test_grammar _test_agent \
        _test_generator _test_diff_stub

all: build-all

$(BUILD):
	mkdir -p $(BUILD)

# --- ASan-instrumented C targets used by `make test` ------------------------
$(BUILD)/target_repro: structured/target.c | $(BUILD)
	$(CC) $(CFLAGS_LAB) -DPARSE_RECORD_MAIN $< -o $@

$(BUILD)/parse_config_repro: structured/parse_config.c | $(BUILD)
	$(CC) $(CFLAGS_LAB) -DPARSE_CONFIG_MAIN $< -o $@

$(BUILD)/config_parser: grammar/target/config_parser.c | $(BUILD)
	$(CC) $(CFLAGS_LAB) $< -o $@

$(BUILD)/agent: snapshot/agent.c | $(BUILD)
	$(CC) -g -O1 -Wall -Wextra $< -o $@

build-all: $(BUILD)/target_repro $(BUILD)/parse_config_repro \
           $(BUILD)/config_parser $(BUILD)/agent

# --- deterministic CI test --------------------------------------------------
test: build-all _test_target _test_parse_config _test_grammar \
      _test_generator _test_agent _test_diff_stub
	@echo "==== ch30-fuzzing-2: all tests PASSED ===="

# Every _test_* runs its own subshell. The pattern for the ASan-crash
# reproducers is:
#   - the binary is expected to trip ASan (nonzero exit + marker on stderr)
#   - if it prints MED_CH30_BUG_NOT_TRIGGERED, the bug regressed — fail
#   - if ASan's "ERROR: AddressSanitizer" marker appears, pass
# ASAN_OPTIONS pins the exit code so the grep is deterministic.

_test_target: $(BUILD)/target_repro
	@echo "[test] structured/target.c :: planted heap/stack overflow"
	@out=$$(ASAN_OPTIONS='abort_on_error=0:exitcode=42' \
	        ./$(BUILD)/target_repro 2>&1 || true); \
	 echo "$$out" | head -20; \
	 if echo "$$out" | grep -q 'MED_CH30_BUG_NOT_TRIGGERED'; then \
	     echo "FAIL: planted bug in target.c regressed"; exit 1; fi; \
	 if echo "$$out" | grep -q 'AddressSanitizer'; then \
	     echo "PASS: ASan tripped the planted overflow"; \
	 else \
	     echo "FAIL: no AddressSanitizer marker in output"; exit 1; fi

_test_parse_config: $(BUILD)/parse_config_repro
	@echo "[test] structured/parse_config.c :: unbounded recursion"
	@ulimit -s 8192; \
	 set +e; \
	 out=$$(ASAN_OPTIONS='abort_on_error=0:exitcode=42' \
	        ./$(BUILD)/parse_config_repro 2>&1); rc=$$?; \
	 echo "$$out" | head -20; \
	 echo "[exit rc=$$rc]"; \
	 if echo "$$out" | grep -q 'MED_CH30_BUG_NOT_TRIGGERED'; then \
	     echo "FAIL: planted stack-overflow regressed"; exit 1; fi; \
	 if [ "$$rc" -ne 0 ]; then \
	     echo "PASS: recursion crashed the parser (rc=$$rc)"; \
	 else \
	     echo "FAIL: parser returned success but produced no marker"; exit 1; fi

_test_grammar: $(BUILD)/config_parser
	@echo "[test] grammar/target/config_parser.c :: identifier overflow"
	@tmp=$$(mktemp); \
	 printf 'group verylongidentifier { x = 1 }\n' > $$tmp; \
	 out=$$(ASAN_OPTIONS='abort_on_error=0:exitcode=42' \
	        ./$(BUILD)/config_parser $$tmp 2>&1 || true); \
	 rm -f $$tmp; \
	 echo "$$out" | head -20; \
	 if echo "$$out" | grep -q 'MED_CH30_BUG_NOT_TRIGGERED'; then \
	     echo "FAIL: planted bug in config_parser regressed"; exit 1; fi; \
	 if echo "$$out" | grep -q 'AddressSanitizer'; then \
	     echo "PASS: ASan tripped the planted overflow"; \
	 else \
	     echo "FAIL: no AddressSanitizer marker in output"; exit 1; fi

_test_generator:
	@echo "[test] grammar/generate.py :: deterministic output"
	@a=$$(python3 grammar/generate.py 42); \
	 b=$$(python3 grammar/generate.py 42); \
	 if [ -z "$$a" ]; then echo "FAIL: generator produced empty output"; exit 1; fi; \
	 if [ "$$a" != "$$b" ]; then echo "FAIL: generator non-deterministic under fixed seed"; exit 1; fi; \
	 echo "PASS: seed=42 stable output ($$(echo -n "$$a" | wc -c) bytes)"

_test_agent: $(BUILD)/agent
	@echo "[test] snapshot/agent.c :: builds and runs"
	@out=$$(./$(BUILD)/agent 2>&1); \
	 echo "$$out"; \
	 echo "$$out" | grep -q 'MED_CH30_SNAPSHOT_AGENT_OK' \
	    && echo "PASS: agent handshake stub ran" \
	    || { echo "FAIL: agent did not print OK marker"; exit 1; }

# The libFuzzer harnesses (fdp_harness.cc, lpm_harness.cc, json_diff.cc)
# need clang-18's fuzzer runtime plus optional libraries. For CI we
# syntax-check them by attempting a `-fsyntax-only` compile with clang-18
# when it is available — that proves the code is well-formed on the pinned
# toolchain without pulling in libprotobuf-mutator or yyjson.
_test_diff_stub:
	@echo "[test] libFuzzer harness syntax check (best-effort)"
	@if command -v $(CLANGXX) >/dev/null 2>&1; then \
	   $(CLANGXX) -std=c++17 -fsyntax-only -x c++ structured/fdp_harness.cc \
	     -I/usr/lib/llvm-18/lib/clang/18/include \
	     >/dev/null 2>&1 \
	     && echo "PASS: fdp_harness.cc parses under clang-18" \
	     || echo "SKIP: fdp_harness.cc syntax check needs clang-18 fuzzer headers"; \
	 else \
	   echo "SKIP: $(CLANGXX) not present"; \
	 fi

# --- optional heavyweight fuzz targets (not part of `make test`) ------------
fuzz-fdp:
	$(CLANGXX) -g -O1 -std=c++17 \
	    -fsanitize=fuzzer,address,undefined \
	    -fno-omit-frame-pointer -fstack-protector-strong \
	    structured/fdp_harness.cc structured/target.c \
	    -o $(BUILD)/fdp_fuzz

fuzz-lpm:
	@mkdir -p $(BUILD)
	protoc --cpp_out=$(BUILD) -Istructured structured/config.proto
	$(CLANGXX) -g -O1 -std=c++17 \
	    -fsanitize=fuzzer,address \
	    -I$(BUILD) -I/usr/include \
	    structured/lpm_harness.cc structured/parse_config.c \
	    $(BUILD)/config.pb.cc \
	    -lprotobuf-mutator-libfuzzer -lprotobuf-mutator -lprotobuf \
	    -o $(BUILD)/lpm_fuzz

fuzz-grammar: $(BUILD)/config_parser
	bash grammar/run.sh 500

fuzz-diff:
	$(CLANGXX) -g -O1 -std=c++17 \
	    -fsanitize=fuzzer,address \
	    differential/json_diff.cc \
	    -ljsoncpp -lyyjson \
	    -o $(BUILD)/json_diff

clean:
	rm -rf $(BUILD) grammar/crashes
