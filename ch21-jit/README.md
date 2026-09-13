# Chapter 21 — companion code

Deliberately vulnerable teaching artifacts and working exploits for
Chapter 21 of *Modern Exploit Development*. **Lab use only** — see
`../ETHICS.md`.

The "vulnerable target" here is a **pinned V8** built from source with
a small teaching patch applied on top; the exploit runs under that
patched `d8`. Nothing here targets a shipping browser.

## Pinned versions

| What | Version |
|------|---------|
| V8 tag | **`12.9.203`** (checked out with `depot_tools`) |
| Host toolchain | Ubuntu 24.04 LTS, gcc 13, clang 18, glibc 2.39 |
| Patch | `patches/lab-type-confusion.patch` (removes one `CheckMaps` in Maglev's specialization for `PACKED_DOUBLE_ELEMENTS` `Array.prototype.at` and the matching store) |

Override with `V8_TAG=<tag> make build` if you want to try a different
tag — but the offsets in `exploit/rw.js` and `test/smoke.js` are cited
against `12.9.203` and will need re-reading with `%DebugPrint` for any
other tag.

## Contents

| Path | What it is |
|------|------------|
| `README.md` | this file |
| `Makefile` | `make build` fetches + builds d8; `make test` runs the exploit and asserts the smoke marker |
| `build.sh` | fetch depot_tools, sync V8 at the pinned tag, apply the teaching patch, build `x64.release` `d8` |
| `run.sh` | invoke the pinned `d8` with `--allow-natives-syntax --expose-gc --no-lazy-feedback-allocation`, load the scripts in order, run the smoke test |
| `patches/lab-type-confusion.patch` | the teaching patch (removed `CheckMaps` in Maglev) |
| `exploit/vuln.js` | Listing `lst-vuln`: warms and Maglev-optimizes `leak` so it drops the map check |
| `exploit/addrof-fakeobj.js` | Listing `lst-primitives`: bit-punning helpers `d2u`/`u2d` and the `addrof`/`fakeobj` primitives |
| `exploit/helpers.js` | `read32`, `elementsAddrOf`, `safeAddr`, `dumpHeader` — bootstrap helpers used by `rw.js` |
| `exploit/rw.js` | Listing `lst-rw` (full source): forges a `Uint32Array` header inside a container, exposes `read64`/`write64` |
| `test/smoke.js` | round-trip a canary through the forged view AND through an independent native `Uint32Array`, then survive a forced GC; prints `CH21-SMOKE-OK` on success |
| `.ci-skip` | one-line reason CI does not run this chapter (see below) |

## Build & run

Requires Linux x86-64, `git`, `python3`, `curl`, `pkg-config`, and
enough disk (~30 GB) and time (~30 minutes on a modern desktop) for a
V8 build.

```bash
# from this directory:
make build          # fetches depot_tools, syncs V8 12.9.203, applies the
                    # teaching patch, builds out.gn/x64.release/d8 and
                    # symlinks it here as ./d8

make test           # runs the exploit under the pinned d8 and asserts
                    # the CH21-SMOKE-OK marker is printed
```

## Expected `make test` output (abridged)

```
$ make test
... d8 output ...
addrof(probe) = <some address, e.g. 0x001c0f19>
DebugPrint: 0x001c0f19: [JS_OBJECT_TYPE]
 - map: 0x00000...
 ...
CH21-SMOKE-OK
ch21-jit: OK
```

`make test` exits **0** only when `CH21-SMOKE-OK` appears in the d8
output — the smoke test prints that marker only after both the
round-trip and post-GC survival assertions pass.

## CI

This chapter is **not exercised by the Linux CI** (see `.ci-skip`):
building V8 12.9.203 with `depot_tools` needs ~30 GB and ~30 minutes,
which is beyond the shared runner budget. All build/run scripts and
the full exploit sources ship in this directory so the chapter is
reproducible on any Linux workstation.

## Chapter cross-references

- Prereqs: {{ref:ch:ch-js-internals}} (V8 object model, elements kinds,
  pointer compression), {{ref:ch:ch-uaf-typeconfusion}} (weaponizing
  type confusion).
- Follow-on: {{ref:ch:ch-rw-to-rce}} (escape the V8 sandbox from the
  arbitrary R/W built here), {{ref:ch:ch-fullchain}} (reliability
  engineering).
