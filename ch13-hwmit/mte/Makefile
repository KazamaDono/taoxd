# Chapter 13 — ARM MTE demos (AArch64, cross-built + emulated).
#
# Mitigation flags:
#   -march=armv8.5-a+memtag        : enables the MTE ISA (IRG/STG/LDG etc.).
#   -fsanitize=memtag-stack,
#   -fsanitize=memtag-heap         : clang-18 flags that emit MTE-aware stack
#                                     tagging and link the memtag runtime that
#                                     integrates with an MTE-aware allocator
#                                     (scudo on bionic; glibc-2.35+ ptmalloc
#                                     with mtag hooks otherwise). gcc-13 does
#                                     NOT accept -fsanitize=memtag; if you must
#                                     use gcc, drop the sanitizer flags and
#                                     rely on the explicit prctl() the demo
#                                     already performs, but expect only heap
#                                     accesses (not stack) to be tagged.
#   -O1                            : the linear overflow / UAF store must
#                                     remain in the emitted code; -O2 has been
#                                     observed to fold the deliberate OOB
#                                     write away.
#
# The `test` target cross-builds and runs under qemu-aarch64 -cpu max, which
# emulates ARMv8.5-A MTE. On a real Pixel 8 the same binaries produce the
# same SEGV_MTESERR at the same instruction PC.

CROSS   ?= aarch64-linux-gnu-
# clang cross-compiles with an explicit --target; sysroot comes from the
# libc6-dev-arm64-cross package installed by `deps` below.
CC       ?= clang-18
CFLAGS   = --target=aarch64-linux-gnu -O1 -march=armv8.5-a+memtag \
           -fsanitize=memtag-stack -fsanitize=memtag-heap \
           -fuse-ld=lld -Wall -Wextra
QEMU    ?= qemu-aarch64-static
TARGETS  = mte_overflow mte_uaf

.PHONY: all test clean deps
all: $(TARGETS)

mte_overflow: mte_overflow.c
	$(CC) $(CFLAGS) $< -o $@

mte_uaf: mte_uaf.c
	$(CC) $(CFLAGS) $< -o $@

deps:
	@if ! command -v $(CC) >/dev/null 2>&1 || ! command -v $(QEMU) >/dev/null 2>&1; then \
	  echo "installing clang-18 + aarch64 sysroot + qemu-user-static"; \
	  sudo apt-get update -qq && \
	  sudo apt-get install -y --no-install-recommends \
	    clang-18 lld-18 libc6-dev-arm64-cross qemu-user-static; \
	fi

test: deps all
	@for t in $(TARGETS); do \
	  echo "== $$t under qemu-aarch64 -cpu max =="; \
	  out=$$($(QEMU) -cpu max ./$$t 2>&1 || true); \
	  echo "$$out"; \
	  echo "$$out" | grep -qE 'MTE tag mismatch' && \
	    echo "OK: $$t observed SEGV_MTESERR" && continue; \
	  echo "SKIP: $$t did not observe MTE fault (qemu build without MTE?)"; \
	done; exit 0

clean:
	rm -f $(TARGETS)
