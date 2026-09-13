# Chapter 13 — Hardware-Enforced Defenses: CET, PAC, and MTE

Companion code for Chapter 13 of *Modern Exploit Development*. **Lab use only** — see `../../ETHICS.md`.

Each demo is written to *trigger* a hardware check so you can watch the fault fire and inspect the `si_code` your handler receives. Nothing here weaponizes the primitive against a third-party target.

## Contents

| Path | What it is | Hardware / emulator |
|------|------------|---------------------|
| `cet/shadow_stack_demo.c` | Overwrites the saved RA and reads `si_code` — `SEGV_CPERR` on CET, `SEGV_MAPERR` elsewhere | Intel Tiger Lake+ / AMD Zen 3+ |
| `cet/ibt_demo.c` | Indirect call that skips `ENDBR64` and traps under IBT | Intel Tiger Lake+ / AMD Zen 3+ |
| `cet/Makefile` | Builds both CET demos with `-fcf-protection=full` | — |
| `cet/run.sh` | Runs both demos with SHSTK on and off, greps for the CET marker | — |
| `pac/pac_demo.c` | Sign with `pacia`, authenticate with `autia`, forge and fault at first deref | AArch64 v8.3-A+ or `qemu-aarch64 -cpu max` |
| `pac/Makefile` | Cross-builds and runs under `qemu-aarch64-static -cpu max` | — |
| `mte/mte_overflow.c` | One-byte linear overflow across an MTE granule → `SEGV_MTESERR` in sync mode | AArch64 v8.5-A+ or `qemu-aarch64 -cpu max` |
| `mte/mte_uaf.c` | Use-after-free caught by scudo's retag-on-free | same |
| `mte/Makefile` | Cross-builds with `-fsanitize=memtag`, runs under qemu-user | — |
| `run_qemu.sh` | Convenience: run the whole demo suite under qemu-user; `--system` boots a full aarch64 VM | — |

## Build & run

From this directory, inside the pinned lab container:

```bash
make test              # build + exercise every demo, prints OK / SKIP per demo
./run_qemu.sh          # same, driven through the qemu wrapper
./run_qemu.sh --system # optional full-system boot with -cpu max,mte=on
```

Individual demos:

```bash
make -C cet test       # SHSTK/IBT on x86-64 (needs CET-capable CPU to fault)
make -C pac test       # PAC sign/auth round-trip + forged-signature fault
make -C mte test       # MTE linear overflow + UAF, both SEGV_MTESERR in sync
```

## Expected `make test` output

On a machine with all three defenses available (CET-capable Intel/AMD, plus qemu-user-static installed):

```
== shadow_stack_demo, SHSTK enabled ==
about to smash the saved return address...
SIGSEGV @ 0xdeadbeef  si_code=10  (SHSTK/IBT (CET) violation)
== shadow_stack_demo, SHSTK disabled ==
about to smash the saved return address...
SIGSEGV @ 0xdeadbeef  si_code=1  (unmapped address)
== ibt_demo ==
indirect call skipping ENDBR64...
SIGSEGV @ ...  si_code=10  (SHSTK/IBT (CET) violation)
OK: observed CET fault (SEGV_CPERR)
== pac_demo under qemu-aarch64 -cpu max ==
HWCAP_PACA = 1
raw    = 0000aaaaaaaa1234
signed = 003c2aaaaaaa1234  (bits[54:48] hold the PAC)
xpaci  = 0000aaaaaaaa1234
good() = 41
forged after autia = 002c3aaaaaaa1234
OK: PAC sign/auth round-trip valid; forged pointer faulted at deref
== mte_overflow under qemu-aarch64 -cpu max ==
a=0x... b=0x... (top-byte tags differ)
SIGSEGV @ ...  si_code=9  (MTE tag mismatch (sync))
OK: mte_overflow observed SEGV_MTESERR
== mte_uaf under qemu-aarch64 -cpu max ==
alloc  stale = ...
realloc fresh = ...
SIGSEGV @ ...  si_code=9  (MTE tag mismatch (UAF))
OK: mte_uaf observed SEGV_MTESERR
ch13-hwmit: all demos exercised (see per-demo OK/SKIP lines)
```

On a runner without one of these (e.g. non-CET CPU, or qemu-user unavailable and the recipe cannot `apt-get install`), the corresponding demo prints `SKIP: ...` and the aggregate `make test` still exits 0 — the build itself validates the toolchain, and `.ci-skip` records that this chapter's fault behavior is hardware-conditional.

## Why `.ci-skip`

Hardware-tag semantics — `SEGV_CPERR`, `SEGV_MTESERR`, PAC's deferred fault — are not reliably reproducible on the shared, virtualized CI hardware GitHub provides. The demos build in CI; observing the actual faults requires either the pinned lab container (which brings its own `qemu-user-static` + cross toolchain) or a physical Tiger Lake+/Zen 3+ / ARMv8.5-A host.
