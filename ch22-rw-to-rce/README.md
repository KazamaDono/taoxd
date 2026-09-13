# Chapter 22 — From Arbitrary R/W to Code Execution in a Hardened Renderer

Companion carriers and exploit for Chapter 22 of *Modern Exploit Development*. **Lab use only** — see [`../ETHICS.md`](../ETHICS.md).

> ⚠️ **`.ci-skip`ped in the open-source cut.** The exploit runs against the pinned, deliberately-weakened `d8` (V8) produced by the Ch 21 lab's `build.sh`. Building V8 needs Google's `depot_tools` and a few hours of CPU time, which is outside the Linux CI runners' budget. Follow Ch 21's build instructions, then run the exploit from this directory.

## Contents

| Path | What it is |
|------|------------|
| `carrier/carrier.js` | Shellcode-carrier constants: encodes native bytes into JS doubles the JIT accepts |
| `exploit.js` | The RW->RCE payload: uses the Ch 21 primitive, plants shellcode into a JIT'd WASM page, hijacks dispatch |
| `.ci-skip` | Marks this chapter as not exercised by the Linux CI (reason inside) |

## Running

```bash
# 1. Build the pinned patched d8 (from Ch 21's build.sh)
../ch21-jit/build.sh

# 2. Run the exploit against that d8
../ch21-jit/build/out.gn/x64.release/d8 --allow-natives-syntax exploit.js
```
