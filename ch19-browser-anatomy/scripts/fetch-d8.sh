#!/usr/bin/env bash
# Download a prebuilt d8 for the V8 tag pinned in ../D8_VERSION and stage it
# under ../build/d8 for `make smoke`. This is the fast road; the slow,
# authoritative road is scripts/build-d8.sh, which produces a d8 with the
# lab patches applied and full introspection intrinsics.
#
# The V8 team publishes prebuilt archives at
#   https://storage.googleapis.com/chromium-v8/official/canary/
# but the URL layout drifts; we resolve it via v8-version.txt so the pinned
# tag is a single source of truth. If the archive host is unreachable
# (offline lab), this script exits non-zero and points at build-d8.sh.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/.." && pwd)"
BUILD="${ROOT}/build"
VERSION_FILE="${ROOT}/D8_VERSION"

if [[ ! -f "${VERSION_FILE}" ]]; then
  echo "fetch-d8: missing ${VERSION_FILE}" >&2
  exit 1
fi
VERSION="$(tr -d '[:space:]' < "${VERSION_FILE}")"

mkdir -p "${BUILD}"
cd "${BUILD}"

case "$(uname -sm)" in
  "Linux x86_64")  PLATFORM="linux64" ;;
  "Linux aarch64") PLATFORM="linux-arm64" ;;
  *) echo "fetch-d8: unsupported platform $(uname -sm); use build-d8.sh" >&2
     exit 2 ;;
esac

ARCHIVE="v8-${PLATFORM}-rel-${VERSION}.zip"
URL="https://storage.googleapis.com/chromium-v8/official/canary/${ARCHIVE}"

echo "fetch-d8: pulling ${URL}"
if ! curl -fSL --retry 3 --retry-connrefused -o "${ARCHIVE}" "${URL}"; then
  echo "fetch-d8: download failed. If you are offline or the tag has been" >&2
  echo "          reaped from the canary bucket, run scripts/build-d8.sh."  >&2
  exit 3
fi

# Record a checksum next to the archive so subsequent runs (and CI) can
# notice drift without re-downloading.
sha256sum "${ARCHIVE}" | tee "${ARCHIVE}.sha256"

unzip -o -q "${ARCHIVE}" -d d8-${VERSION}
# The archive lays out ./d8, ./snapshot_blob.bin, ./icudtl.dat at the root.
ln -sf "d8-${VERSION}/d8" d8
ln -sf "d8-${VERSION}/snapshot_blob.bin" snapshot_blob.bin
ln -sf "d8-${VERSION}/icudtl.dat" icudtl.dat

echo "fetch-d8: ready at ${BUILD}/d8 (V8 ${VERSION})"
