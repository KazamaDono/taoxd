# Ch 33: build the pre/post binaries for the diffing exercise. Actual
# diffing is a manual step in Ghidra + BinDiff/Diaphora; see .ci-skip.
.PHONY: all clean help
CC ?= gcc
all: build/parser_v1 build/parser_v2 build/harness

build:
	mkdir -p build

# v1 is the vulnerable version; v2 is the patched one.
build/parser_v1: src/parser_v1.c | build
	$(CC) -O2 -fno-stack-protector -no-pie -o $@ $< 2>/dev/null || echo "  (needs src/parser_v1.c)"
build/parser_v2: src/parser_v2.c | build
	$(CC) -O2 -fno-stack-protector -no-pie -o $@ $< 2>/dev/null || echo "  (needs src/parser_v2.c)"
build/harness: src/harness.c | build
	$(CC) -O2 -o $@ $< 2>/dev/null || echo "  (needs src/harness.c)"

help:
	@echo "Ch 33: patch-diff n-day analysis."
	@cat .ci-skip

clean:
	@rm -rf build
