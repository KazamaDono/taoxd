# Book demos — the exact scripts that produce the terminal figures in the book

Every Ubuntu-style terminal screenshot printed in *The Art of Exploit Development, 2nd Edition* is a real capture of one of these scripts running on the author's own laptop. Nothing here is simulated — you can run each one and reproduce the output verbatim.

## Running them

Everything here is pure Python 3.11+ or Node.js 20+. **No compilation, no lab image, no root.** These demos are the *arithmetic* behind exploitation techniques — they show the mangling, packing, layout, and math without requiring the vulnerable targets or a Linux runtime.

```bash
python3 demos/ch14-safe-linking.py     # glibc 2.32+ tcache mangle/demangle round-trip
python3 demos/ch07-intwrap.py          # 32-bit size_t wrap → heap overflow
python3 demos/ch10-libc-rebase.py      # one leak, one subtraction, every libc symbol resolved
python3 demos/ch05-cyclic.py           # De Bruijn pattern → offset lookup
python3 demos/ch08-rop-chain.py        # hand-assembled ret2libc chain + on-the-wire bytes
python3 demos/ch09-srop-frame.py       # forged rt_sigframe with per-register offsets
python3 demos/ch11-canary-brute.py     # byte-by-byte canary brute vs a fork-server (simulated)
python3 demos/ch13-pac.py              # ARM PAC bit layout + autia failure
node    demos/ch20-nan-boxing.mjs      # NaN-boxing: real 64-bit bit patterns of JS values
```

## What each demo covers

| Chapter | Demo | Concept |
|---|---|---|
| 5 | `ch05-cyclic.py` | De Bruijn cyclic pattern; find the offset of the saved return address without counting |
| 7 | `ch07-intwrap.py` | Integer overflow in an allocation size collapsing into a heap overflow |
| 8 | `ch08-rop-chain.py` | Building a ret2libc chain by hand — leak, rebase, gadgets, `/bin/sh`, `system` |
| 9 | `ch09-srop-frame.py` | Forging a `struct rt_sigframe` so `rt_sigreturn` loads *your* register state |
| 10 | `ch10-libc-rebase.py` | Turning one leaked pointer into the addresses of every other libc symbol |
| 11 | `ch11-canary-brute.py` | Byte-at-a-time canary recovery against a fork-server; 8-byte canary in ~1000 probes |
| 13 | `ch13-pac.py` | ARM PAC anatomy: a signed pointer, the autia check, and what happens when it fails |
| 14 | `ch14-safe-linking.py` | glibc 2.32+ `(pos >> 12) ^ next` mangling, and why a heap-address leak is now required |
| 20 | `ch20-nan-boxing.mjs` | How JS engines pack pointers, integers, and doubles into one 64-bit slot |

These are the pedagogical demos — the *runnable exploit labs* live in the chapter directories (`ch05-stack/`, `ch08-rop/`, etc.) and require the pinned Ubuntu 24.04 lab image (see the repo root `README.md`).

## Lab-only, defensive framing

These scripts illustrate the arithmetic and bit-level layout of exploitation techniques. They do not attack any live system. See `../ETHICS.md`.
