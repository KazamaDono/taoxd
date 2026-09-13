# Deriving the buffer-to-saved-RIP offset

The Ch10 leak-server exploit hard-codes an offset of **72** bytes from the
start of `handle()`'s `name[64]` buffer to the saved return address of
`handle()`.  This note records how the number was derived so that future
readers can re-derive it if the build flags change.

## The frame layout

With the current build (see `Makefile`), `handle()` compiles to a frame
laid out as:

```text
    +----------------------+  <- rsp on entry to `read()`
    |  name[64]            |  64 bytes
    +----------------------+
    |  saved rbp           |  8 bytes  (frame pointer)
    +----------------------+
    |  saved rip           |  8 bytes  <- overwrite target
    +----------------------+
```

Sixty-four bytes of `name` plus eight bytes of saved `rbp` gives an
offset of **72** from `&name[0]` to the return-address slot.

The System V AMD64 ABI requires `rsp` to be 16-byte aligned at every
`call` boundary, so gcc will size the prologue's `sub rsp, N` so that
after the push of the return address (`call` pushes 8), the local frame
lands on a 16-byte boundary.  A 64-byte buffer plus an 8-byte saved
`rbp` gives N = 72, which lands exactly on the required alignment.  If
you change `name`'s size, or add another local, verify the offset is
still 72 (or update it in `solve.py::find_offset`).

## Empirical derivation with `cyclic`

The offset above was originally *found* (not assumed) with pwntools'
`cyclic`/`cyclic_find` pair.  The recipe is worth remembering:

```python
from pwn import cyclic, cyclic_find, process
io = process("./vuln")
io.recvuntil(b"name? ")
io.send(cyclic(200) + b"\n")           # send a de Bruijn pattern
io.wait()                              # SIGSEGV expected
core = io.corefile                     # pwntools grabs the coredump
offset = cyclic_find(core.read(core.rsp, 4))
print("offset =", offset)              # -> 72
```

The four bytes at `rsp` after the fault are the tail of the cyclic
pattern that landed in the return slot; `cyclic_find` recovers the
index into the pattern where those bytes lived, and that index is the
buffer-to-RIP distance.

## Sensitivity

The offset moves whenever any of the following changes:

1. **Buffer size** — the `char name[64]` declaration in `vuln.c`.
2. **Frame-pointer omission** — building with `-fomit-frame-pointer` drops
   the saved-`rbp` slot, taking the offset from 72 to 64.
3. **Extra locals** — adding a local variable to `handle()` may grow the
   frame past 72 to keep alignment.
4. **Optimization level** — `-O2` may inline `handle()` into `main()` or
   `banner()` into `handle()`, changing the frame entirely.  The
   Makefile pins `-O1 -fno-inline` for this reason.

Whenever you touch any of the above, re-run the cyclic snippet above
and update `find_offset()` in `solve.py`.
