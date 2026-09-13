# Chapter 21 — JIT: addrof/fakeobj
#
# This chapter's "target" is a pinned V8 build with the teaching patch
# from patches/lab-type-confusion.patch applied. We do NOT ship a
# prebuilt d8; build.sh fetches the pinned tag with depot_tools and
# builds it. The build is heavyweight (~30 min, ~30 GB) so CI skips
# this chapter via .ci-skip; run `make test` locally on Linux.
#
# Mitigation posture (documented for the reader; set inside build.sh
# via the v8gen.py args):
#
#   is_debug              = false       -- release build (mitigations ON)
#   v8_enable_sandbox     = true        -- V8 heap sandbox ENABLED
#                                          (the R/W in ch21 is confined
#                                          to the compressed heap on
#                                          purpose; ch22 escapes it)
#   v8_enable_pointer_compression = true -- 32-bit compressed pointers
#                                           (required for the addrof
#                                           low-32-bits trick)
#   symbol_level          = 1           -- symbols for --print-maglev-*
#
# The teaching *bug* is inserted by the patch, not by disabling a
# mitigation. That distinction matters and the chapter is explicit
# about it.

HERE       := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
D8         ?= $(HERE)/d8
V8_TAG     ?= 12.9.203
SMOKE_MARK := CH21-SMOKE-OK

.PHONY: all build test clean

all: build

# `make build` -- fetch, patch, and build the pinned d8.
build:
	V8_TAG=$(V8_TAG) bash $(HERE)/build.sh

# `make test` -- run the exploit under the pinned d8 and assert the
# smoke marker is printed. Exits 0 only on success.
test:
	@if [ ! -x "$(D8)" ]; then \
	  echo "ch21-jit: pinned d8 not found at $(D8); run 'make build' first" >&2; \
	  exit 2; \
	fi
	@out="$$(bash $(HERE)/run.sh 2>&1)"; \
	 echo "$$out"; \
	 echo "$$out" | grep -q '$(SMOKE_MARK)' || { \
	   echo "ch21-jit: smoke marker '$(SMOKE_MARK)' not found; test FAILED" >&2; \
	   exit 1; \
	 }; \
	 echo "ch21-jit: OK"

clean:
	rm -f $(HERE)/d8
	rm -rf $(HERE)/.v8
