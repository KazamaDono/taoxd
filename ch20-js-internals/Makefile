# Chapter 20 — JavaScript Engine Internals for Exploitation
#
# There is no C target to compile in this chapter: every "target" is the
# V8 d8 shell, built once by ../ch19-browser-anatomy/build-v8.sh and
# pinned to the 12.4 branch as declared in bible/tech-stack.md.  The
# scripts in this directory only make sense when executed against that
# shell with --allow-natives-syntax enabled, which is why this chapter
# is marked `.ci-skip` for the shared runner.
#
# Mitigation-flag note (there is nothing to compile here, but the pinned
# d8 is built with the following flags — recorded here so the book and
# repo agree on the assumed target):
#
#   v8_enable_pointer_compression = true    # the 4 GiB heap cage of Ch20
#   v8_enable_sandbox            = true    # external-pointer table on
#   v8_enable_debugging_features = true    # keeps %DebugPrint / %HaveSameMap
#   is_debug                     = false   # release build; symbols kept
#
# Turning either of the first two OFF invalidates every listing in this
# chapter (pointers become uncompressed 64-bit, the cage-base tag stops
# existing).  Do not.

SCRIPTS := object_layout.js butterfly_probe.js typedarray_layout.js \
           maps_and_transitions.js
DOCS    := README.md pointer_compression_notes.md
D8      ?= ../ch19-browser-anatomy/v8/out/x64.release/d8
MARKER  := CH20_OK

.PHONY: all build test run-all clean help

all: build

# There is nothing to compile.  "build" verifies the artefacts are on
# disk and syntactically consistent with a d8 invocation.
build:
	@for f in $(SCRIPTS); do \
	  test -f $$f || { echo "missing: $$f" >&2; exit 1; }; \
	  grep -q '%DebugPrint\|%HaveSameMap' $$f \
	    || { echo "$$f: no natives-syntax intrinsic found" >&2; exit 1; }; \
	done
	@for f in $(DOCS); do \
	  test -f $$f || { echo "missing: $$f" >&2; exit 1; }; \
	done
	@test -x run.sh || { echo "run.sh not executable" >&2; exit 1; }
	@echo "ch20-js-internals: build OK ($(words $(SCRIPTS)) scripts, $(words $(DOCS)) docs)"

# `make test` is deterministic and does NOT require d8: it validates
# structural invariants of every lab script (correct filename, expected
# intrinsics present, wrapper still shells out to d8 the way the book
# says).  When d8 IS available on $PATH or at $D8, `make run-all` will
# execute every script for real; that path is what a reader running the
# chapter locally invokes, and is what the .ci-skip note points at.
test: build
	@# 1. every script mentions the exact filename the book cites for it
	@for f in $(SCRIPTS); do \
	  head -3 $$f | grep -q "$$f" \
	    || { echo "$$f: banner comment does not name the file" >&2; exit 1; }; \
	done
	@# 2. wrapper still invokes d8 with the required flag
	@grep -q -- '--allow-natives-syntax' run.sh \
	  || { echo "run.sh no longer passes --allow-natives-syntax" >&2; exit 1; }
	@# 3. cross-reference: object_layout.js must exercise %HaveSameMap,
	@#    butterfly_probe.js must drive Smi->Double, typedarray_layout.js
	@#    must dump both the view and the buffer.
	@grep -q '%HaveSameMap' object_layout.js
	@grep -q '1\.5'         butterfly_probe.js
	@grep -q 'ArrayBuffer(0x1000)' typedarray_layout.js
	@grep -q '%DebugPrint(u8)'     typedarray_layout.js
	@grep -q '%DebugPrint(buf)'    typedarray_layout.js
	@# 4. dictionary-mode transition exercise present in the aux script
	@grep -q 'delete d\.' maps_and_transitions.js
	@echo "$(MARKER): ch20-js-internals structural test passed"

# Optional: run every script through the pinned d8 if one is available.
# Never invoked by CI (see .ci-skip); useful locally.
run-all:
	@test -x "$(D8)" || { \
	  echo "d8 not built at $(D8); run ../ch19-browser-anatomy/build-v8.sh" >&2; \
	  exit 3; }
	@for f in $(SCRIPTS); do \
	  echo "==== $$f ===="; \
	  "$(D8)" --allow-natives-syntax $$f; \
	done

clean:
	@echo "nothing to clean"

help:
	@echo "targets: build (default), test, run-all, clean"
	@echo "  build   : verify scripts/docs are present and well-formed"
	@echo "  test    : deterministic structural test (CI-safe, no d8)"
	@echo "  run-all : execute every script under the pinned d8 (\$$D8=$(D8))"
