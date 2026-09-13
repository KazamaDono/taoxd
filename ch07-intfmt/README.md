# Chapter 7 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 7,
*Format String and Integer Bugs*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch07-intfmt test        # build the target(s) and run the exploit test
```

Every runnable lab ships a `make test` that asserts the exploit reaches its
goal; CI runs it on every push (x86-64 **and** arm64).

## Contents

| Path | What it is |
|------|------------|
| `src/fmt.c` | Format-string target: `printf(buf)` with a global `authorized` write target. Built `-no-pie -fno-stack-protector` and **without** FORTIFY so `%n` works. |
| `solve/solve_fmt.py` | pwntools solve: auto-discovers the positional offset, then `fmtstr_payload` writes `authorized = 1` and pops a shell. |
| `src/intmul.c` | Integer-overflow target: a 32-bit size multiply wraps to an undersized `malloc`, then `fread` overflows into an adjacent `auth` object (data-only win). |
| `solve/solve_intmul.py` | pwntools solve: picks `count` so the size calc wraps to `0x40`, overflows the victim's `is_admin` flag, asserts `ACCESS GRANTED`. |
| `src/intmul_fixed.c` | Remediated version: C23 `<stdckdint.h>` `ckd_mul` rejects the overflowing size (full source; the chapter prints only the excerpt). Falls back to `__builtin_mul_overflow` on gcc 13, which predates the header. |
| `Makefile` | Builds the targets with explicit, commented mitigation flags (`-no-pie`, no stack protector, FORTIFY off and **why**), plus a `sanitizers` demo target and the deterministic `test`. |

## What the build flags mean

The `fmt` target is compiled **without** `_FORTIFY_SOURCE` on purpose — Ubuntu's
gcc enables `_FORTIFY_SOURCE=3` by default when optimizing, which rewrites
`printf` to `__printf_chk` and refuses `%n` in a writable format. Disabling it is
what makes the `%n` arbitrary write reachable at all. The `intmul` target keeps
FORTIFY at Ubuntu's default **on** to make the point of exercise 4: FORTIFY does
nothing for an integer-overflow size bug. Every flag is commented in the
`Makefile`.

## Expected `make test` output

```text
== [1/3] format string: printf(buf) -> %n write -> shell ==
[*] '/work/ch07-intfmt/fmt'
[*] controlled argument at offset 8
[+] Starting local process '/work/ch07-intfmt/fmt': pid ...
[+] format string -> arbitrary write -> code execution (uid 0)
== [2/3] integer overflow -> heap overflow -> data-only win ==
[+] Starting local process '/work/ch07-intfmt/intmul': pid ...
[+] integer overflow -> heap overflow -> privilege flag set
== [3/3] remediation: ckd_mul rejects the overflowing size ==
OK: fixed build refused the wrapped size
ALL CH07 TESTS PASSED
```

The offset printed at step 1 (`8` on the `-no-pie -O0` x86-64 build) is
**discovered at runtime**, not hardcoded — it differs on AArch64 and on other
build flags, which is the whole point of `find_offset`. The reported `uid`
depends on who runs the test (it is `0` only inside the root lab container); the
test asserts that the spawned shell *ran our command*, not any particular uid.

## Extra: the sanitizer demos

```bash
make -C ch07-intfmt sanitizers   # needs clang-18 (in the lab image)
```

Builds `intmul.ubsan` (`-fsanitize=unsigned-integer-overflow,implicit-integer-truncation`,
both **Clang** UBSan checks) and `intmul.asan` (`-fsanitize=address`). Feed either
the 12-byte overflow header to watch UBSan catch the wrap at the multiply and
ASan catch the `heap-buffer-overflow` at the `fread`.
