# Chapter 9 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 9,
*Advanced Code Reuse: JOP, COP, and Friends*. **Lab use only** — see
`../ETHICS.md`. Every target is built with **explicit, commented mitigation
flags** (see the `Makefile` header); every runnable lab ships a `make test`
that asserts the exploit reaches its goal, and CI runs it on every push.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                   # drop into the Ubuntu 24.04 lab container
make -C ch09-codereuse test        # build the target(s) and run the exploit test
```

## Contents

| Path | Listing | What it is |
|------|---------|------------|
| `jop/find_jop.sh` | lst-jop-find | Enumerate JOP/COP (indirect-branch) gadgets for a target with `ropper` + `ROPgadget`. |
| `srop/vuln.c` | lst-srop-vuln | Leak-free SROP target: linear stack overflow + exported `syscall;ret` / `pop rax;ret` gadgets + a fixed `"/bin/sh"`. |
| `srop/solve.py` | lst-srop-solve | Full SROP exploit: forge a pwntools `SigreturnFrame` → `execve("/bin/sh")`, complete register control from one gadget. |
| `srop/vuln_arm64.c` | — | AArch64 port of the SROP target (syscall 139 via `x8`, planted `svc #0` gadgets). Full source; not printed inline. |
| `srop/solve_arm64.py` | — | AArch64 SROP solve (Exercise 5), run under `qemu-aarch64`. Full source; not printed inline. |
| `dlresolve/vuln.c` | — | Partial-RELRO, lazy-binding ret2dlresolve target (planted `pop rdi/rsi/rdx` gadgets, since glibc ≥ 2.34 has no `__libc_csu_init`). Full source; not printed inline. |
| `dlresolve/solve.py` | lst-dlresolve-solve | Leak-free ret2dlresolve: `system("/bin/sh")` via `Ret2dlresolvePayload`, never learns a libc address. |
| `csu/csu_probe.sh` | lst-csu-probe | Probe whether `__libc_csu_init`'s universal gadget is still present (absent for glibc ≥ 2.34). |

`lst-jop` and `lst-decision`/`tbl-techniques` are a figure and a table; `lst-csu`
is representative disassembly with no repo path.

## What `make test` does

Deterministic, exits 0 **only** on success. Each exploit runs under a `timeout`
and must print a unique marker **and** a real `id` line from a spawned shell:

1. **SROP** (`srop/`) — forge one `SigreturnFrame`, `execve("/bin/sh")`; expects `SROP_OK` + `uid=`.
2. **ret2dlresolve** (`dlresolve/`) — resolve and call `system("/bin/sh")` with no libc leak; expects `DLR_OK` + `uid=`.
3. **csu_probe** — asserts the probe reports `__libc_csu_init` is **gone** on glibc 2.39 (the modern Ubuntu 24.04 outcome).
4. **find_jop** — asserts the gadget-enumeration script runs and emits its report.
5. **AArch64 SROP** under `qemu-aarch64` — best effort, **non-fatal**: runs when the cross toolchain + qemu-user are present, skipped cleanly otherwise so it never breaks the x86-64 gate.

### Expected output (abridged)

```
########## 1. SROP: one forged frame -> shell ##########
[+] Starting local process './vuln': pid ...
[+] shell: uid=1000(...) gid=1000(...) groups=...
ok: SROP forged-frame execve spawned a shell

########## 2. ret2dlresolve: system("/bin/sh"), no leak ##########
[*] Loaded ... cached gadgets for './vuln'
[+] shell: uid=1000(...) gid=1000(...) groups=...
ok: ret2dlresolve called system without a leak

########## 3. csu_probe: __libc_csu_init absent on glibc 2.39 ##########
[-] no __libc_csu_init (glibc >= 2.34 build).
    Use ret2dlresolve, a libc setcontext gadget, or libc-internal
    arg-setting gadgets located after a leak.
ok: probe correctly reports the universal gadget is gone (>= glibc 2.34)

########## 4. find_jop: enumerate indirect-branch gadgets ##########
== dispatcher candidates (advance + indirect jmp) ==
...
ok: find_jop.sh ran and produced its JOP/COP report

########## 5. AArch64 SROP under qemu-user (best effort) ##########
[+] offset=...  shell: uid=0(root) ...
ok: AArch64 SROP spawned a shell under qemu-user

ch09-codereuse: all checks passed — make test OK
```

## Notes & exercises

- **Exercise 3** (Full RELRO kills ret2dlresolve): rebuild with `-Wl,-z,relro -Wl,-z,now` and rerun `dlresolve/solve.py` — it fails because `-z now` resolves every import at startup, marks the whole GOT read-only, and removes the lazy `PLT[0]` path the exploit abuses.
- **Exercise 5** (AArch64 SROP): `srop/solve_arm64.py` is the port — syscall `139` in `x8`, frame fields `x0`/`pc`/`sp`.
- The ret2dlresolve target plants `pop rdi/rsi/rdx; ret` gadgets on purpose: on glibc ≥ 2.34 the `__libc_csu_init` arg-setting gadgets are gone, so a realistic small-binary chain still needs a few planted gadgets to drive the resolver — it just needs no libc leak.
