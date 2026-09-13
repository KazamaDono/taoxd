# Chapter 20 — JavaScript Engine Internals for Exploitation

Companion lab for Chapter 20 of *Modern Exploit Development*. Every
script here is a `d8` transcript from the chapter: run each one under
the pinned V8 shell to see the object layouts the prose describes.

**Lab use only** — see `../ETHICS.md`.

## Pinned target

- V8 branch **12.4** (same pin as Chapters 19, 21, 22) — see
  `../../bible/tech-stack.md`.
- Built with `v8_enable_pointer_compression = true` and
  `v8_enable_sandbox = true` (both are on by default). The 4 GiB heap
  cage of the chapter's figures only exists under those flags.
- The `%DebugPrint`, `%HaveSameMap`, and `%SystemBreak` intrinsics
  require `--allow-natives-syntax`; the `run.sh` wrapper always passes
  it. They do not exist in a browser process, and none of these
  scripts should ever be pointed at one.

## Building d8

The pinned shell is built by the Chapter 19 helper:

```bash
( cd ../ch19-browser-anatomy && ./build-v8.sh )
# produces ../ch19-browser-anatomy/v8/out/x64.release/d8
```

Point the wrapper at a different build with `D8=/path/to/d8 ./run.sh …`.

## Contents

| Path                            | What it is                                                                                                             |
|---------------------------------|------------------------------------------------------------------------------------------------------------------------|
| `Makefile`                      | Structural `make test` (no d8 required); `make run-all` invokes every script under the pinned shell.                   |
| `run.sh`                        | Thin wrapper: `d8 --allow-natives-syntax <script.js>`.                                                                 |
| `object_layout.js`              | Listing 20-1 verbatim. Builds two identically-shaped `{x, y}` objects, dumps them with `%DebugPrint`, and asserts `%HaveSameMap`. |
| `butterfly_probe.js`            | Listing 20-3 verbatim. Grows an array past its inline capacity (forces a `FixedArray` realloc) then transitions the elements kind `PACKED_SMI` → `PACKED_DOUBLE` (forces a re-typed copy). Watch the `elements:` pointer move. |
| `typedarray_layout.js`          | Listing 20-4 verbatim. Allocates a 4 KiB `ArrayBuffer`, wraps it in a `Uint8Array`, and dumps both — showing the compressed in-cage view header and the external `backing_store` pointer that escapes the cage. |
| `maps_and_transitions.js`       | Exercise scaffolding for Ex. 2 and Ex. 6. Compares `{x,y}` vs `{y,x}` shapes and walks a `FastProperties → DictionaryProperties` transition via `delete`. |
| `pointer_compression_notes.md`  | Reference sheet: tag bits, cage-base arithmetic, JSObject / FixedArray / JSArrayBuffer offsets, JSC NaN-boxing, SpiderMonkey notes, and pointers into the pinned V8 source tree. |
| `.ci-skip`                      | Marks this directory as skipped by the shared CI runner (a V8 build is too heavy). Local `make test` still passes without d8. |

## Running

Structural test (CI-safe, no d8 required):

```bash
$ make test
ch20-js-internals: build OK (4 scripts, 2 docs)
CH20_OK: ch20-js-internals structural test passed
```

Real run against the pinned d8:

```bash
$ make run-all
==== object_layout.js ====
DebugPrint: 0x1e2a00041b9d: [JSObject]
 - map: 0x1e2a08281f8d <Map(HOLEY_ELEMENTS)> [FastProperties]
 - prototype: 0x1e2a00040f0d <Object map = 0x1e2a08281f39>
 - elements: 0x1e2a00002259 <FixedArray[0]> [HOLEY_ELEMENTS]
 - properties: 0x1e2a00002259 <FixedArray[0]>
 - All own properties (excluding elements): {
    0x1e2a000029c5: [String] in ReadOnlySpace: #x: 1 (const data field 0), location: in-object
    0x1e2a000029d5: [String] in ReadOnlySpace: #y: 2 (const data field 1), location: in-object
 }
DebugPrint: 0x1e2a00041ba9: [JSObject]
 - map: 0x1e2a08281f8d <Map(HOLEY_ELEMENTS)> [FastProperties]
 ...
same map: true
==== butterfly_probe.js ====
... (three %DebugPrint dumps of `arr`; `elements:` address changes
     between the second and third, and the Map's elements kind flips
     from PACKED_SMI_ELEMENTS to PACKED_DOUBLE_ELEMENTS on the last)
==== typedarray_layout.js ====
0x1e2a...: [JSTypedArray]
 - map: 0x1e2a... <Map(UINT8_ELEMENTS)>
 - byte_offset: 0
 - byte_length: 4096
 - buffer: 0x1e2a... <ArrayBuffer map = ...>
0x1e2a...: [JSArrayBuffer]
 - backing_store: 0x7fXXXXXXXXXX
 - byte_length: 4096
 ...
==== maps_and_transitions.js ====
--- {x,y} vs {x,y} ---
... xy1 same-map as xy2? true
--- {x,y} vs {y,x} ---
... xy1 same-map as yx? false
--- dictionary transition ---
... [FastProperties]
... [DictionaryProperties]
dict same-map as fresh {x,y}? false
```

Exact addresses (and the cage-base prefix) vary per run because of
ASLR; the *shape* of each dump — the elements kind, the same-map
booleans, the elements-address change on growth — is what matters and
is what the chapter walks through.

## CI

This directory ships `.ci-skip` because reproducing a full V8 12.4
build inside the shared runner costs tens of minutes and multi-GB of
disk. `make test` still runs green offline and asserts every script
matches the shape the book cites (correct filename banner, expected
intrinsics, wrapper flags). Substantive execution is a local step.
