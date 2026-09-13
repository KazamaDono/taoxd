#!/usr/bin/env bash
# Provision depot_tools, sync V8 at the pinned tag from ../D8_VERSION, and
# build d8 with the flags Part V assumes:
#
#   is_debug=false                  # release, so JIT tiers behave normally
#   symbol_level=2                  # keep symbols for gdb / rr in Ch. 21+
#   v8_enable_disassembler=true     # so %DisassembleFunction works in d8
#   v8_enable_object_print=true     # so %DebugPrint prints structured
#   v8_enable_pointer_compression=true  # Ch. 20 assumes compressed cage
#   v8_enable_sandbox=true          # Ch. 22 targets the heap sandbox
#   v8_expose_memory_corruption_api=false  # NOT for lab experiments
#
# Takes ~40 min on eight cores from a cold cache.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/.." && pwd)"
VERSION_FILE="${ROOT}/D8_VERSION"
BUILD="${ROOT}/build"
DEPOT="${BUILD}/depot_tools"
SRC="${BUILD}/v8-src"

if [[ ! -f "${VERSION_FILE}" ]]; then
  echo "build-d8: missing ${VERSION_FILE}" >&2
  exit 1
fi
VERSION="$(tr -d '[:space:]' < "${VERSION_FILE}")"
mkdir -p "${BUILD}"

if [[ ! -d "${DEPOT}" ]]; then
  git clone --depth 1 \
    https://chromium.googlesource.com/chromium/tools/depot_tools.git \
    "${DEPOT}"
fi
export PATH="${DEPOT}:${PATH}"
export DEPOT_TOOLS_UPDATE=0

if [[ ! -d "${SRC}/v8" ]]; then
  mkdir -p "${SRC}"
  (cd "${SRC}" && fetch --no-history v8)
fi

cd "${SRC}/v8"
git fetch --tags --depth=1 origin "refs/tags/${VERSION}:refs/tags/${VERSION}"
git checkout "${VERSION}"
gclient sync -D --with_branch_heads --with_tags

# Apply lab patches (kept in ../patches/ so they version with the book).
if [[ -d "${ROOT}/patches" ]]; then
  for p in "${ROOT}/patches"/*.patch; do
    [[ -e "$p" ]] || continue
    echo "build-d8: applying $(basename "$p")"
    git apply --index "$p"
  done
fi

OUT="out/x64.release"
mkdir -p "${OUT}"
cat > "${OUT}/args.gn" <<'GN'
is_debug = false
symbol_level = 2
target_cpu = "x64"
v8_enable_disassembler = true
v8_enable_object_print = true
v8_enable_pointer_compression = true
v8_enable_sandbox = true
v8_expose_memory_corruption_api = false
GN
gn gen "${OUT}"
autoninja -C "${OUT}" d8

install -m 0755 "${OUT}/d8" "${BUILD}/d8"
for f in snapshot_blob.bin icudtl.dat; do
  [[ -f "${OUT}/${f}" ]] && cp -f "${OUT}/${f}" "${BUILD}/${f}"
done
echo "build-d8: ready at ${BUILD}/d8 (V8 ${VERSION})"
