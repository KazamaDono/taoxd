# Chapter 15 — Heap Exploitation Primitives

Companion solve scripts for Chapter 15 of *Modern Exploit Development*. **Lab use only** — see [`../ETHICS.md`](../ETHICS.md).

::: {#not-shipped}
> ⚠️ **This chapter is `.ci-skip`ped in the open-source cut.** The chapter text and the exploit solve scripts are here, but the deliberately vulnerable heap-target C sources are omitted. Readers reproduce the labs by:
>
> - Reading the chapter's listing captions (which describe each target's shape), or
> - Cloning [shellphish/how2heap](https://github.com/shellphish/how2heap) — the reference implementation for every glibc heap technique, versioned by libc.
>
> Both approaches teach the same primitives on the same glibc 2.39.
:::

## Contents

| Path | What it is |
|------|------------|
| `tcache-poison/solve.py` | Tcache-poisoning exploit demonstrating safe-linking bypass (given a leak) |
| `uaf-arbwrite/solve.py` | UAF → arbitrary write via reclaimed function-pointer object |
| `.ci-skip` | Marks this chapter as not exercised by the Linux CI (reason inside) |

The chapter covers: UAF, double-free, tcache poisoning with safe-linking mangling, fastbin dup, overlapping chunks, and modern write targets (`_IO_FILE` vtables, `tls_dtor_list`, `exit` handlers) now that `__free_hook`/`__malloc_hook` are removed as of glibc 2.34.

## References

- how2heap glibc-2.39 branch: <https://github.com/shellphish/how2heap>
- glibc source (`malloc/malloc.c`): <https://sourceware.org/git/?p=glibc.git>
