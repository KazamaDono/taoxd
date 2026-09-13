# Chapter 12 — companion code

Deliberately vulnerable teaching artifacts and working exploits for
Chapter 12 (*Control-Flow Integrity: CFG, XFG, and CFI*) of
*Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh              # drop into the Ubuntu 24.04 lab container
make -C ch12-cfi test         # build the targets and run every check
```

Every runnable lab ships a `make test` that asserts the observed behavior
matches the chapter's claims; CI runs it on every push.

## Contents

| Path | What it is |
|------|------------|
| `cfi-icall/demo.c`    | Listing 12.1 verbatim. Clang `cfi-icall` **rejects** a mistyped indirect call. |
| `cfi-icall/reuse.c`   | Listing 12.2 verbatim. Clang `cfi-icall` **accepts** a same-signature swap — the "allowed-target reuse" leak. |
| `cfi-icall/Makefile`  | Builds `demo` (recovering CFI), `reuse` (trapping CFI), and `demo-nocfi` (no CFI, for comparison). Every mitigation flag is commented with which defense it toggles and why. |
| `xfg/xfg-notes.md`    | Windows-only reference: MSVC `/guard:xfg` build recipe, `dumpbin /LOADCONFIG` walkthrough, and the type-hash-prefix inspection of Listing 12.3. Not run by CI. |
| `kcfi/kcfi-probe.sh`  | Bash + `objdump` scanner that finds the `mov imm32 / add [reg-4] / jne` kCFI prologue-check pattern of Listing 12.4 in Linux 6.8 modules. `--self-test` generates a synthetic ELF and probes it, so CI exercises the matcher without a kernel. |

## Mitigation matrix (what is on/off in each binary)

| Binary | LTO | `-fsanitize=cfi` | trap or recover | purpose |
|--------|-----|------------------|-----------------|---------|
| `cfi-icall/demo`         | on  | on  | recover (diagnostic to stderr) | show the check firing |
| `cfi-icall/reuse`        | on  | on  | trap (production default)      | show the check *passing* on a same-type swap |
| `cfi-icall/demo-nocfi`   | off | off | n/a                            | show what would happen without CFI |

## Expected `make test` output (abridged)

```
$ make -C ch12-cfi test
=== [1/2] demo: CFI must REJECT the mistyped call ===
legal call: 5
about to make the illegal call...
demo.c:24:13: runtime error: control flow integrity check for type
  'int (int, int)' failed during indirect function call
OK: cfi-icall diagnostic observed

=== [2/2] reuse: CFI must ACCEPT the same-signature swap ===
[!!! admin_shell reached, fd=3, arg=hello]
OK: same-type reuse passed CFI (as designed)

=== kCFI probe self-test ===
[+] scanning /tmp/tmp.XXXX/probe.o
[+] disassembling with objdump -d --no-show-raw-insn
[+] found 1 kCFI check sites (mov imm32 / add [reg-4] / jne)
[+] example:
    0: mov    $0x12345678,%eax
    5: add    -0x4(%r11),%eax
    9: jne    12

ch12-cfi: PASS
```

## What the test *proves*

1. **`demo`** — Clang `cfi-icall` produces the "control flow integrity
   check for type ..." diagnostic when a function pointer typed
   `int(*)(int,int)` is redirected to a `void(*)(const char*)`. This is
   the fine-grained end of the coarseness axis of Table 12.1.
2. **`reuse`** — the same defense, with default trap-on-failure, does
   **not** fire when the swap is to another function of the *same*
   signature. This is the equivalence-class leak that motivates COOP
   and every "same-set reuse" bypass in the chapter.
3. **`kcfi-probe --self-test`** — the objdump-based pattern matcher
   for the Linux 6.8 kCFI prologue check works on a synthesized ELF.
   Point it at a real `CONFIG_CFI_CLANG=y` module for Exercise 4.

## Not exercised by CI

- `xfg/` is Windows/MSVC-only; a Windows 11 24H2 workstation is
  required to reproduce Listing 12.3. The notes in `xfg/xfg-notes.md`
  are step-by-step.

CI-eligible on Ubuntu 24.04 x86-64: `cfi-icall/` and `kcfi/` (self-test).
