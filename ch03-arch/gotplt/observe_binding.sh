#!/usr/bin/env bash
# Observe lazy vs eager binding through the dynamic loader's own tracing.
# No debugger required.  Ubuntu 24.04 / glibc 2.39.
set -euo pipefail

echo "== default (lazy): puts is bound when first called =="
LD_DEBUG=bindings ./greeter 2>&1 >/dev/null | grep "\`puts'" | head -n 1 || true

echo "== LD_BIND_NOW=1 (eager): every symbol is bound before main =="
LD_BIND_NOW=1 LD_DEBUG=bindings ./greeter 2>&1 >/dev/null \
    | grep -E "\`(puts|printf)'" | head -n 2 || true
