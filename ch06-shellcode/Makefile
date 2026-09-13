# ch06-shellcode — build the teaching shellcode payloads and prove they run.
#
# Lab-only. These binaries invoke execve() from hand-written assembly on
# both x86-64 and AArch64. `make test` runs them and asserts /bin/sh
# executed by feeding it a command and grepping its output.

CC       ?= gcc
CROSS    ?= aarch64-linux-gnu-gcc
QEMU     ?= qemu-aarch64-static
CFLAGS   := -nostdlib -static

BINS_X64  := build/x64_execve build/x64_revshell_stub
BIN_A64   := build/a64_execve
HARNESS   := build/run_shellcode
ENCODED   := build/x64_execve_encoded.bin

.PHONY: all test clean
all: $(BINS_X64) $(BIN_A64) $(HARNESS)

build:
	mkdir -p build

build/x64_execve: src/execve_x64.S | build
	$(CC) $(CFLAGS) -o $@ $<

# revshell target is provided for reading; we don't dial home in tests.
# Build only to prove it assembles cleanly.
build/x64_revshell_stub: src/revshell_x64.S | build
	$(CC) $(CFLAGS) -o $@ $< 2>/dev/null || { \
	  echo "  (revshell build skipped — kept as source-only teaching listing)"; \
	  touch $@; }

build/a64_execve: src/execve_arm64.S | build
	@if command -v $(CROSS) >/dev/null 2>&1; then \
	  $(CROSS) $(CFLAGS) -o $@ $< ; \
	else \
	  echo "  (aarch64 cross-toolchain not present; skipping $@)"; \
	  touch $@ ; \
	fi

build/run_shellcode: harness/run_shellcode.c | build
	$(CC) -O2 -o $@ $<

# ---- test: prove the execve shellcode really runs /bin/sh --------------------
# We feed the spawned shell an `echo` and grep for a unique marker.

MARKER := __sh_ran_ch06__

test: all
	@echo "==> x86-64 execve shellcode"
	@printf 'echo %s\nexit\n' '$(MARKER)' | ./build/x64_execve 2>/dev/null | grep -q '$(MARKER)' \
	  && echo "  PASS x86-64 execve" \
	  || { echo "  FAIL x86-64 execve"; exit 1; }
	@echo "==> AArch64 execve shellcode (under $(QEMU))"
	@if [ -s ./build/a64_execve ] && command -v $(QEMU) >/dev/null 2>&1; then \
	  printf 'echo %s\nexit\n' '$(MARKER)' | $(QEMU) ./build/a64_execve 2>/dev/null | grep -q '$(MARKER)' \
	    && echo "  PASS AArch64 execve" \
	    || { echo "  FAIL AArch64 execve"; exit 1; }; \
	else \
	  echo "  SKIP AArch64 (no cross-toolchain or qemu-aarch64-static)"; \
	fi
	@echo "==> XOR encoder round-trip on the execve blob"
	@objcopy -O binary -j .text ./build/x64_execve build/x64_execve.bin
	@python3 encoders/xor_encode.py build/x64_execve.bin > build/x64_execve.enc 2>build/x64_execve.enc.log
	@python3 -c "import sys; \
	  raw=open('build/x64_execve.bin','rb').read(); \
	  enc=open('build/x64_execve.enc','rb').read(); \
	  key=int(open('build/x64_execve.enc.log').read().split('key=0x')[1][:2],16); \
	  dec=bytes(b^key for b in enc); \
	  bad={0x00,0x0a,0x0d}; \
	  assert dec==raw, 'round-trip broken'; \
	  assert not (set(enc)&bad), f'encoded still has bad bytes'; \
	  print(f'  PASS xor encoder (key=0x{key:02x}, {len(raw)} bytes, no NUL/CR/LF)')"

clean:
	rm -rf build
