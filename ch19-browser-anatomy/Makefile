# Chapter 19 — Anatomy of a Modern Browser and Its Sandbox
#
# The Part V d8 experiments need a pinned V8 build (fetch-d8 or build-d8).
# The trust-boundary lesson lives in the toy Mojo-shaped broker under
# toy-ipc/, which is what CI exercises via `make test`.
#
# Mitigation-flag rationale (this chapter is architectural — the "toggles"
# are policy toggles on the *broker*, not compile-time -f flags on a target):
#
#   ALLOWED_PREFIXES  ON  : broker enforces an allowlist; without this,
#                           any renderer request would be honored — the
#                           shape of a real broker-side logic escape.
#   realpath()        ON  : symlink resolution BEFORE the prefix check
#                           closes the TOCTOU pattern behind several real
#                           Chromium sandbox escapes.
#   65536-byte cap    ON  : bounded message size; the corresponding OFF
#                           setting is the unbounded-allocation DoS class.
#   send_fds (SCM)    ON  : broker attaches an OS handle to the reply so
#                           the renderer never calls open() itself.
#
# Deterministic: `make test` seeds /tmp/toy-ipc-lab/ with fixed contents,
# runs the driver, and greps the unique marker TOY_IPC_ALL_OK.

PY      ?= python3
ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD   := $(ROOT)/build
D8      := $(BUILD)/d8
DRIVER  := $(ROOT)/toy-ipc/driver.py
PKGROOT := $(ROOT)

.PHONY: all test toy-ipc smoke fetch-d8 build-d8 clean help

all: help

help:
	@echo "Chapter 19 targets:"
	@echo "  make test        # run the toy-ipc broker demo; asserts TOY_IPC_ALL_OK"
	@echo "  make toy-ipc     # same, but streams full output"
	@echo "  make fetch-d8    # download the pinned prebuilt d8 into build/"
	@echo "  make build-d8    # build d8 from source at the pinned V8 tag"
	@echo "  make smoke       # run scripts/smoke.js against build/d8"
	@echo "  make clean       # remove build/ and generated symlinks"

# ---- CI target -------------------------------------------------------------
# Must exit 0 ONLY on success. We grep for the unique marker; drift in any
# of the three scripted outcomes leaves the marker absent and fails the run.
test:
	@rm -rf /tmp/toy-ipc-lab
	@PYTHONPATH="$(PKGROOT):$$PYTHONPATH" \
	  $(PY) $(DRIVER) | tee $(ROOT)/.last-test.log
	@grep -q '^TOY_IPC_ALL_OK$$' $(ROOT)/.last-test.log \
	  || { echo "FAIL: marker TOY_IPC_ALL_OK not found" >&2; exit 1; }
	@echo "PASS: ch19 toy-ipc trust-boundary demo"

toy-ipc:
	@PYTHONPATH="$(PKGROOT):$$PYTHONPATH" $(PY) $(DRIVER)

# ---- Optional Part-V bring-up (not run by CI) ------------------------------
fetch-d8:
	@bash $(ROOT)/scripts/fetch-d8.sh

build-d8:
	@bash $(ROOT)/scripts/build-d8.sh

smoke: $(D8)
	@$(D8) --allow-natives-syntax $(ROOT)/scripts/smoke.js

$(D8):
	@echo "No d8 at $(D8). Run 'make fetch-d8' (fast) or 'make build-d8' (slow)." >&2
	@exit 1

clean:
	rm -rf $(BUILD) $(ROOT)/.last-test.log $(ROOT)/toy_ipc
