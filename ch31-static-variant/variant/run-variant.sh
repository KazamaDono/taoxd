#!/usr/bin/env bash
# End-to-end variant hunt driver — Listing 31-4.
#
# Assumes `codeql` (2.17+) is on PATH.  Run from the variant/ directory:
#
#     bash run-variant.sh
#
# The script builds the whole ch31 tree (target/ + variant/) via the parent
# Makefile, extracts a CodeQL C/C++ database that covers every translation
# unit, and then runs the NetworkMemcpy taint query twice: once as a
# regression check that the original seed bug is still flagged, and once
# as the actual sweep for variants.
set -euo pipefail

CH31_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DB="$CH31_ROOT/variant/variant-db"
QUERY="$CH31_ROOT/codeql/queries/NetworkMemcpy.ql"

# 1. Build the database from the seeded codebase (target/ + variant/).
rm -rf "$DB"
codeql database create "$DB" \
    --language=cpp \
    --command="make -C $CH31_ROOT -B build" \
    --source-root="$CH31_ROOT"

# 2. Confirm the query catches the *known* bug first.
codeql database analyze "$DB" "$QUERY" \
    --format=csv --output="$CH31_ROOT/variant/known.csv"
grep -q "handler.c" "$CH31_ROOT/variant/known.csv" \
    || { echo "regression: seed bug in target/handler.c not flagged"; exit 1; }

# 3. Sweep for variants across the whole tree.
codeql database analyze "$DB" "$QUERY" \
    --format=sarif-latest --output="$CH31_ROOT/variant/variants.sarif"

# 4. Extract sink locations for triage.  The .bqrs path is stable relative
#    to the database but its parent directory encodes the query's qlpack
#    name, so glob rather than hard-code it.
BQRS="$(find "$DB/results" -name 'NetworkMemcpy.bqrs' -print -quit)"
codeql bqrs decode --format=csv "$BQRS" \
    | tee "$CH31_ROOT/variant/variants.csv"
