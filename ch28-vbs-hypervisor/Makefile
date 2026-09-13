# ch28-vbs-hypervisor/Makefile
#
# Builds the toy MMIO "device model" as a shared library and runs the
# Python solver from exploit/ under pytest, asserting that the out-of-
# bounds write from the mock guest actually transfers control to win()
# on the host side (CH28-ESCAPED marker) and that AddressSanitizer
# flagged the OOB.
#
# ---------------------------------------------------------------------
# Mitigation posture (documented on purpose)
# ---------------------------------------------------------------------
#
#   -fsanitize=address              ON. The whole pedagogical point of
#                                   the lab is to see the OOB write into
#                                   the device-state struct. ASan makes
#                                   the report loud and unambiguous, in
#                                   the same way you would run QEMU with
#                                   -fsanitize=address when hunting real
#                                   device-model bugs.
#
#   -fno-stack-protector            OFF (protector disabled). The OOB is
#                                   a heap/global-buffer overflow inside
#                                   a struct, not a stack overflow, so
#                                   the canary would never fire; leaving
#                                   it off keeps generated code closer
#                                   to what a real VMM's per-transaction
#                                   handler compiles to and stops
#                                   -D_FORTIFY_SOURCE from tripping on
#                                   the deliberate over-length memcpy.
#
#   -U_FORTIFY_SOURCE               OFF. Ubuntu 24.04's default is
#                                   _FORTIFY_SOURCE=3, which would emit
#                                   a runtime __memcpy_chk() around the
#                                   buggy copy and abort before the OOB
#                                   actually lands. That would defeat
#                                   the demonstration.
#
#   -fPIC + -shared                 ON. Required to load the lab as
#                                   libtoylab.so from Python via ctypes.
#                                   ASLR on the loader stays ENABLED;
#                                   the exploit resolves win() through
#                                   dlsym(), not a hardcoded address, so
#                                   randomisation is not in our way.
#
#   -O0 -g                          ON. Preserves the struct layout the
#                                   book relies on (rx_slot immediately
#                                   followed by on_complete) so the OOB
#                                   lands exactly where the prose says.
#
# `make test` is deterministic and self-contained; the recipe installs
# pytest into a venv if it isn't already on PATH.
# ---------------------------------------------------------------------

CC       ?= gcc
CFLAGS   := -O0 -g -std=gnu11 -Wall -Wextra \
            -fPIC -fsanitize=address \
            -fno-stack-protector -U_FORTIFY_SOURCE
LDFLAGS  := -shared -fsanitize=address

LIB      := toy-device/libtoylab.so
SRC      := toy-device/device.c toy-device/guest.c toy-device/lab_main.c
HDR      := toy-device/device.h

VENV     := .venv
PYTEST   := $(VENV)/bin/pytest

.PHONY: all test clean deps

all: $(LIB)

$(LIB): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# Provision pytest into a chapter-local venv. On Ubuntu 24.04 PEP 668
# forbids a plain `pip install --user` against the system Python, so we
# use a venv and keep everything inside this chapter directory (no
# touching root Dockerfile / CI / requirements).
deps:
	@test -x $(PYTEST) || ( \
	  python3 -m venv $(VENV) && \
	  $(VENV)/bin/pip install --quiet --upgrade pip && \
	  $(VENV)/bin/pip install --quiet pytest )

test: all deps
	@echo "[+] driving toy device via exploit/solve.py under ASan"
	@cd exploit && ../$(PYTEST) -q test_solve.py
	@echo "PASS: ch28 toy VM-escape reproduced (CH28-ESCAPED marker seen)"

clean:
	rm -f $(LIB)
	rm -rf exploit/__pycache__ exploit/.pytest_cache $(VENV)
