#!/usr/bin/env bash
# Fetch the pinned V8 tag with depot_tools, apply the teaching patch,
# and build d8 x64.release. Heavyweight (~30 min, ~30 GB) — not run in
# CI; see ../.ci-skip. Run on a Linux workstation with a good network.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
V8_TAG="${V8_TAG:-12.9.203}"          # pinned tag; matches README.md
WORK="${WORK:-$HERE/.v8}"             # scratch dir; not committed
JOBS="${JOBS:-$(nproc)}"

mkdir -p "$WORK"
cd "$WORK"

# 1. depot_tools -----------------------------------------------------------
if [[ ! -d depot_tools ]]; then
  git clone --depth 1 \
    https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi
export PATH="$WORK/depot_tools:$PATH"

# 2. fetch v8 at the pinned tag -------------------------------------------
if [[ ! -d v8 ]]; then
  fetch --nohooks v8
fi
cd v8
git fetch --tags origin
git checkout "tags/$V8_TAG" -B "lab-$V8_TAG"
gclient sync -D --with_branch_heads --with_tags

# 3. apply the teaching patch ---------------------------------------------
if ! git apply --check "$HERE/patches/lab-type-confusion.patch" 2>/dev/null; then
  echo "[build.sh] patch already applied or fuzzy; continuing" >&2
else
  git apply "$HERE/patches/lab-type-confusion.patch"
fi

# 4. build d8 x64.release --------------------------------------------------
tools/dev/v8gen.py x64.release -- \
  is_debug=false \
  target_cpu=\"x64\" \
  v8_enable_sandbox=true \
  v8_enable_pointer_compression=true \
  symbol_level=1
ninja -C out.gn/x64.release -j"$JOBS" d8

# 5. surface the resulting binary for run.sh -------------------------------
ln -sf "$WORK/v8/out.gn/x64.release/d8" "$HERE/d8"
echo "[build.sh] d8 built: $HERE/d8"
"$HERE/d8" --version
