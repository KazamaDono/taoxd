# Chapter 14 — companion code

Deliberately instrumented, non-weaponized teaching artifacts for Chapter 14
of *Modern Exploit Development*: chunk anatomy, the bin taxonomy, tcache,
and safe-linking on glibc 2.39. **Lab use only** — see `../ETHICS.md`.

Nothing in this directory exploits anything. The two programs are
observation tools: they print the raw bytes glibc writes into the heap so
you can watch the invariants of Chapter 14 hold in real time.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                            # Ubuntu 24.04 lab container
make -C ch14-heap-internals test            # build + test everything here
```

Or per-subdir:

```bash
make -C ch14-heap-internals/safelink   test
make -C ch14-heap-internals/playground test
```

Requires glibc 2.39 / gcc 13 / Ubuntu 24.04. Safe-linking (glibc 2.32+) is
what the demystifier depends on; on any older libc the `reveal` lines will
be wrong for a reason unrelated to your build.

## Contents

| Path | What it is | Book anchor |
|------|------------|-------------|
| `safelink/safelink.c` | 60-line demystifier: allocs three tcache chunks, frees them, prints raw mangled `fd` next to `REVEAL_PTR` output. Exhibits the tail-sentinel `H0>>12` heap leak. | Listing 14-3 (`lst-safelink`) |
| `safelink/Makefile` | `-O0 -g -no-pie` build; `make test` runs the binary and asserts every reveal line matches its expected pointer plus the tail-leak sentinel. | — |
| `playground/playground.c` | CLI (`alloc N`, `free i`, `dump i`, `tcache`, `bins`, `leak i`, `quit`) exposing chunk metadata, tcache counts, and safe-linked `fd` for interactive experiments. Includes the metadata dumper of Listing 14-5 verbatim. | Listing 14-5 (`lst-playground`) |
| `playground/Makefile` | Builds the playground; `make test` drives it through `script.in` under `setarch -R` and asserts the flag, tcache-count, and tail-sentinel invariants. | — |
| `playground/script.in` | Deterministic command script the test drives through the CLI (four allocs, three frees, tcache/leak dumps). | — |
| `pwndbg/walkthrough.gdb` | gdb script that runs the playground to the post-three-frees breakpoint and issues `heap` / `tcachebins` / `vis_heap_chunks`. Reproduces the session of Listing 14-4. | Listing 14-4 (`lst-pwndbg`) |

Chapter listings that quote glibc source (Listing 14-1 `malloc_chunk` and
Listing 14-2 `tcache_entry`) intentionally have no mirror file here — they
live in the glibc 2.39 tree at `malloc/malloc.c` and are shipped by the
distro. Grep the chapter's `{{ref:...}}` anchors against this table to map
each in-book listing to the file it cites.

## Expected `make test` output (abridged)

```
=== ch14: safelink test ===
c=0x…  raw c->next=0x…  reveal=0x…  (expect b=0x…)
b=0x…  raw b->next=0x…  reveal=0x…  (expect a=0x…)
a=0x…  raw a->next=0x…  reveal=(nil)  (expect NULL)
heap base leak from tail: 0x… (== a>>12: 0x…)
--- assertions ---
OK reveal lines
OK tail-leak line format
OK tail-leak equals a>>12
PASS ch14-safelink

=== ch14: playground test ===
alloc[0] size=0x30 user=0x…
alloc[1] size=0x30 user=0x…
alloc[2] size=0x30 user=0x…
alloc[3] size=0x30 user=0x…
dump[0] (allocated=yes):
user=0x…  hdr=0x…
  prev_size = 0x0
  size      = 0x41  (real 0x40, flags: P )
  next.size = 0x41  (its P bit reflects US: in-use)
free[0] …
free[1] …
free[2] …
tcache counts (non-zero bins):
  bin[ 2] chunk_sz=0x40  count=3
leak[2] user=0x… raw_fd=0x… reveal=0x…
leak[1] user=0x… raw_fd=0x… reveal=0x…
leak[0] user=0x… raw_fd=0x… reveal=0x…   # reveal == user>>12  (tail sentinel)
--- assertions ---
OK alloc[0]
OK alloc[3]
OK PREV_INUSE flag on live chunk
OK neighbor P bit reflects in-use
OK tcache bin[2] count=3
OK tail-leak == user>>12 (safe-linking tail sentinel)
PASS ch14-playground

PASS ch14-heap-internals (all subdirs)
```

Raw addresses vary per run when the container has ASLR enabled; the
assertions compute the expected `user>>12` and compare, so the test is
deterministic without pinning addresses.

## Interactive: driving the playground under gdb / pwndbg

```bash
gdb -x pwndbg/walkthrough.gdb playground/playground
# or, freehand:
gdb ./playground/playground
(gdb) run < ./playground/script.in
```

Inside pwndbg after the third `free` the useful commands are `heap`,
`tcachebins`, `bins`, and `vis_heap_chunks` — the exact set walked in
Listing 14-4 of the chapter.
