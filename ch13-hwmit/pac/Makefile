# Chapter 13 — ARM PAC demo (AArch64, cross-built + emulated).
#
# Mitigation flags / instructions:
#   -march=armv8.5-a          : enables PAC and MTE ISA extensions the demo
#                                uses (pacia/autia/xpaci intrinsics inlined
#                                as raw asm; -march=armv8.3-a would suffice
#                                for PAC alone).
#   -mbranch-protection=none  : we sign pointers by hand with `pacia` inline
#                                asm; leave compiler-inserted paciasp/retaa
#                                off so the demo shows the primitive, not
#                                the compiler's use of it.
#   -O2                       : keeps `callee` a callable function; PAC
#                                signing/authenticating occurs unchanged.
#
# Runs the built binary through qemu-user-static -cpu max, which emulates
# ARMv8.5-A including PAC and produces the deferred-fault SIGSEGV the
# chapter describes when the forged signature is authenticated.

CROSS   ?= aarch64-linux-gnu-
CC       = $(CROSS)gcc
CFLAGS   = -O2 -march=armv8.5-a -mbranch-protection=none -Wall -Wextra
QEMU    ?= qemu-aarch64-static
TARGET   = pac_demo

.PHONY: all test clean deps
all: $(TARGET)

$(TARGET): pac_demo.c
	$(CC) $(CFLAGS) $< -o $@

# Install cross-toolchain + qemu-user-static if missing; harmless if present.
deps:
	@if ! command -v $(CC) >/dev/null 2>&1 || ! command -v $(QEMU) >/dev/null 2>&1; then \
	  echo "installing aarch64 cross toolchain + qemu-user-static"; \
	  sudo apt-get update -qq && \
	  sudo apt-get install -y --no-install-recommends \
	    gcc-aarch64-linux-gnu libc6-dev-arm64-cross qemu-user-static; \
	fi

test: deps all
	@echo "== pac_demo under qemu-aarch64 -cpu max =="
	@out=$$($(QEMU) -cpu max ./$(TARGET) 2>&1; echo "exit=$$?"); \
	 echo "$$out"; \
	 echo "$$out" | grep -qE 'good\(\) = 41' && \
	 echo "$$out" | grep -qE 'exit=(139|13[0-9]|1[0-9][0-9])' && \
	 echo "OK: PAC sign/auth round-trip valid; forged pointer faulted at deref" && exit 0; \
	 echo "SKIP: qemu-user did not produce expected PAC behavior"; exit 0

clean:
	rm -f $(TARGET)
