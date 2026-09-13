# XFG reference build & dumpbin walkthrough

Windows-only. This directory is **not** exercised by the Linux CI (see
`../.ci-skip` if the whole chapter is skipped; the cfi-icall lab still runs on
Linux). It ships as a reference so a reader on Windows 11 24H2 with Visual
Studio 2022 can reproduce Listing 12.3 (`{{ref:lst-xfg}}`) end-to-end.

## Toolchain

- Windows 11 24H2, x64
- Visual Studio 2022 (v17.8+), Desktop C++ workload
- MSVC toolset with `cl.exe` >= 19.38
- `dumpbin.exe` and `link.exe` from the same toolset
- Enable `/guard:xfg` (implies `/guard:cf`); XFG is opt-in and requires an
  XFG-aware Windows build to fully enforce at runtime.

## Minimal target (`target.c`)

```c
#include <stdio.h>

typedef int (*cb_t)(void *, size_t);

static int cb_ok(void *p, size_t n) { return (int)(n & 0x7fff); }

int main(void) {
    cb_t f = cb_ok;
    printf("%d\n", f((void*)0, 42));
    return 0;
}
```

## Build

Open a "x64 Native Tools Command Prompt for VS 2022" and:

```bat
cl /nologo /O2 /GS /guard:xfg /Zi target.c /link /guard:xfg /debug /out:target.exe
```

Key flags:

- `/guard:xfg`  — emit the 8-byte type hash before every address-taken
  function and route indirect calls through
  `__guard_xfg_dispatch_icall_fptr`.
- `/guard:cf`   — implied by `/guard:xfg`; still emits the CFG bitmap so
  older loaders fall back gracefully.
- `/GS`         — stack canary; unrelated but always on.
- `/link /guard:xfg` — mark the PE image `IMAGE_GUARD_XFG_ENABLED` in the
  load-config directory so ntdll turns enforcement on at load time.

## Inspecting the PE

```bat
dumpbin /LOADCONFIG target.exe
```

Look for these lines in `Guard Flags`:

```
    CF Instrumented
    CF Function table present
    CF Enable export suppression
    XFG Enabled
```

`XFG Enabled` is the bit ntdll checks. If it is missing, the loader will
happily downgrade to plain CFG semantics and every same-address-taken
function will pass the check.

## Reading the type-hash prefix

Disassemble a single function and its 8 bytes of hash prefix:

```bat
dumpbin /DISASM /RAWDATA:BYTES target.exe > target.disasm.txt
```

Scan for `cb_ok`. The 8 bytes immediately before the entry point are the
XFG hash for its prototype. In an XFG-instrumented indirect call site you
will also see, roughly:

```
    mov     r10, <same 8 bytes>
    call    qword ptr [__guard_xfg_dispatch_icall_fptr]
```

That `r10` load is Listing 12.3's "expected hash for this site" -- the
dispatcher reads `[rax - 8]` and compares.

## What to notice

1. Two different-prototype functions have two different 8-byte prefixes.
2. Two *same-prototype* functions (e.g. two `int(void*, size_t)` handlers)
   share the same prefix -- this is the reuse surface Chapter 12 discusses.
3. Non-XFG DLLs loaded into an XFG process do **not** get a prefix; calls
   *into* them are still filtered by CFG, but calls *from* them are not
   filtered at all. `dumpbin /LOADCONFIG` on each loaded DLL tells you
   which are protected.

## Automating the check across a directory

Save as `check-xfg.cmd` and run against `C:\Windows\System32`:

```bat
@echo off
for %%F in (%1\*.dll) do (
    dumpbin /LOADCONFIG "%%F" 2>nul | findstr /C:"XFG Enabled" >nul && (
        echo XFG:  %%F
    ) || (
        echo none: %%F
    )
)
```

This is the "which shipped DLLs still lack XFG in your build?" audit that
Exercise 3 of the chapter asks you to perform.
