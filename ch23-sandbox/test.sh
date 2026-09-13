#!/usr/bin/env bash
# ch23-sandbox/test.sh — thin wrapper around `make test` for humans
# who prefer a script. CI invokes `make test` directly.
set -euo pipefail
cd "$(dirname "$0")"
make clean
make test
