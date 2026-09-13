#!/usr/bin/env bash
# Wrapper that invokes the pinned V8 12.4 d8 shell with --allow-natives-syntax
# on the given script.  Every lab script in this directory is meant to be
# executed via:
#
#     ./run.sh object_layout.js
#
# The pinned d8 is the one built by ../ch19-browser-anatomy/build-v8.sh; if you
# have your own build, point $D8 at it explicitly:
#
#     D8=/path/to/v8/out/x64.release/d8 ./run.sh butterfly_probe.js
#
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <script.js> [extra d8 args...]" >&2
  exit 2
fi

script=$1
shift

: "${D8:=../ch19-browser-anatomy/v8/out/x64.release/d8}"

if [[ ! -x "$D8" ]]; then
  cat >&2 <<EOF
error: d8 not found or not executable at: $D8

Build the pinned V8 12.4 shell first:
  ( cd ../ch19-browser-anatomy && ./build-v8.sh )

Or override with:
  D8=/absolute/path/to/d8 $0 $script
EOF
  exit 3
fi

exec "$D8" --allow-natives-syntax "$@" "$script"
