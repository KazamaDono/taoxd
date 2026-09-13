# ch08-rop/target/Makefile - build the deliberately vulnerable lab targets.
#
# MITIGATION FLAGS ARE THE TEACHING SURFACE. Each is explicit and commented so
# you know exactly which defense is on or off and WHY. These binaries are
# INSECURE ON PURPOSE — lab use only (see ../../ETHICS.md).
#
#   -fno-stack-protector  canary OFF  -> the overflow is not detected, so the
#                                        saved return address is reachable.
#   -no-pie               PIE   OFF  -> fixed image base (0x400000); in-binary
#                                        gadget / PLT / GOT addresses are static,
#                                        so Stage-1 needs no binary leak.
#   NX / DEP              LEFT ON     -> the stack is non-executable (gcc's
#                                        default; we do NOT pass -z execstack).
#                                        Injected shellcode is dead; ROP is the
#                                        only way forward. That is the point.
#   -fcf-protection=none  CET   OFF  -> no endbr64/shadow-stack, so the plain
#                                        ROP of this chapter is not pre-empted
#                                        by IBT (CET is Chapter 13).
#   -O0 -g                            -> predictable frames + symbols for labs.
#
# ASLR for the shared libraries is deliberately LEFT ON (realistic): the
# exploit must *leak* libc, it cannot hardcode it. Tests pin the run with
# `setarch -R` only to make stack alignment deterministic.

HOST        := $(shell uname -m)
CC          ?= gcc
AARCH64_CC  := $(shell command -v aarch64-linux-gnu-gcc 2>/dev/null)

COMMON_CFLAGS := -O0 -g -fno-stack-protector -no-pie -fcf-protection=none \
                 -Wall -Wno-unused-function

# On an aarch64 host the native compiler already targets aarch64; on an x86-64
# host we use the cross compiler. Either way the arm64 target is optional.
ifeq ($(HOST),aarch64)
  ARM_CC := $(CC)
else
  ARM_CC := $(AARCH64_CC)
endif

# What can this host build? x86-64 objects need an x86-64-capable gcc.
X86_BUILDABLE := $(shell $(CC) -dumpmachine 2>/dev/null | grep -q x86_64 && echo yes)

TARGETS :=
ifeq ($(X86_BUILDABLE),yes)
  TARGETS += vuln-x86_64 vuln_pivot-x86_64
endif
ifneq ($(ARM_CC),)
  TARGETS += vuln-arm64
endif

.PHONY: all clean
all: $(TARGETS)
	@echo "target: built [$(TARGETS)]"

# --- x86-64 -----------------------------------------------------------------
# rop_gadgets.S guarantees `pop rdi ; ret` & friends exist at fixed in-binary
# addresses even on glibc >= 2.34 (which dropped __libc_csu_init). See the file.
vuln-x86_64: vuln.c rop_gadgets.S
	$(CC) $(COMMON_CFLAGS) -o $@ vuln.c rop_gadgets.S
	@echo "built $@ (NX on, canary off, PIE off, CET off)"

vuln_pivot-x86_64: vuln_pivot.c rop_gadgets.S
	$(CC) $(COMMON_CFLAGS) -o $@ vuln_pivot.c rop_gadgets.S
	@echo "built $@ (NX on, canary off, PIE off, CET off)"

# --- AArch64 ----------------------------------------------------------------
# Same mitigation posture; -fcf-protection is x86-only so it is omitted here
# (PAC/BTI are Chapter 13 and are left off by gcc's default -mbranch-protection).
vuln-arm64: vuln.c rop_gadgets_arm64.S
	$(ARM_CC) -O0 -g -fno-stack-protector -no-pie -Wall -o $@ vuln.c rop_gadgets_arm64.S
	@echo "built $@ (NX on, canary off, PIE off; AArch64 epilogue gadgets linked)"

clean:
	rm -f vuln-x86_64 vuln_pivot-x86_64 vuln-arm64
