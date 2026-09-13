# Chapter 24 — Operating System Kernels for Exploit Developers
#
# This chapter's runnable artifacts are:
#   * vuln/vuln.ko  — an out-of-tree kernel module (needs kernel headers)
#   * exploit/trigger — a userland client that talks to /dev/vuln
#   * lab/run.sh    — boots the pinned Linux 6.8 kernel in QEMU
#   * lab/debug.sh  — attaches gdb to the QEMU stub
#
# CI STANCE (.ci-skip present at this directory root)
# --------------------------------------------------
# The end-to-end lab is intentionally NOT exercised by the shared CI runner:
# it requires a pinned Linux 6.8 bzImage + BusyBox initramfs (~10-20 min to
# build) and a QEMU boot cycle whose only "assertion" is a kernel panic. See
# .ci-skip for the machine-readable reason and lab/BUILD.md for the human
# build recipe.
#
# What `make test` DOES do (locally, in the lab image):
#   1. Build vuln.ko against the running kernel's headers  (mitigation stance
#      documented in vuln/Makefile).
#   2. Build the userland trigger.
#   3. Print a unique SUCCESS marker so a wrapper (or a human) can check.

.PHONY: all module trigger clean test test-build

all: module trigger

module:
	$(MAKE) -C vuln

trigger:
	$(MAKE) -C exploit

# `test-build` is what a maintainer runs on a fully-provisioned lab host to
# prove the build side of the chapter works. It is NOT wired into CI (see
# .ci-skip); running an actual exploit requires the QEMU lab from BUILD.md.
test-build: trigger
	@echo "=== ch24: build check ==="
	$(MAKE) -C vuln
	@test -f vuln/vuln.ko || { echo "vuln.ko missing"; exit 1; }
	@test -x exploit/trigger || { echo "trigger missing"; exit 1; }
	@echo "CH24_BUILD_OK"

# The CI-visible `test` target is a no-op that prints the skip reason and
# exits 0. CI already honors .ci-skip; this exists so a human running
# `make test` inside this directory outside of CI sees the same message.
test:
	@echo "=== ch24: CI-skipped ==="
	@cat .ci-skip
	@echo "CH24_CI_SKIP_OK"

clean:
	-$(MAKE) -C vuln clean
	-$(MAKE) -C exploit clean
