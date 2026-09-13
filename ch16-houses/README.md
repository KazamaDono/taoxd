# Chapter 16 — The House Techniques, Modernized

Companion solve scripts for Chapter 16 of *Modern Exploit Development*. **Lab use only** — see [`../ETHICS.md`](../ETHICS.md).

> ⚠️ **`.ci-skip`ped in the open-source cut.** The chapter text, the safe-linking helper, and the exploit solve scripts are here. The pinned deliberately vulnerable target binaries — House-of-Botcake and House-of-Apple-2 harnesses — are omitted from this initial cut of the repo. The reference implementations live in [shellphish/how2heap](https://github.com/shellphish/how2heap) (`glibc_2.39` branch) and reproduce the same techniques the chapter describes.

## Contents

| Path | What it is |
|------|------------|
| `tools/safe_linking.py` | Safe-linking mangling calculator (`(pos >> 12) ^ ptr`) for glibc 2.32+ |
| `botcake/solve.py` | House-of-Botcake solve template — tcache + unsorted-bin overlap |
| `apple2/solve.py` | House-of-Apple 2 solve template — `_IO_wfile_jumps` FSOP |
| `.ci-skip` | Marks this chapter as not exercised by the Linux CI |

## Which houses survive on glibc 2.39?

- **House of Botcake** — works (tcache stashing + double-free trigger).
- **House of Apple 2** — works (`_IO_wfile_seekoff` still reachable via `_IO_wfile_jumps`).
- **House of Kiwi / House of IO** — work with the FSOP-via-exit path.
- **House of Force** — **dead** since the `top` chunk size check landed in 2.29.
- **House of Orange** — **dead** since the `_IO_str_jumps` scrub in 2.28.
- **Unsorted-bin attack for `global_max_fast`** — largely dead; the write primitive still exists but the FSOP targets it enabled are what you want now.

The chapter's viability matrix is the source of truth per libc version.

## References

- Angel Boy, "HITCON CTF 2018 — Baby Tcache & related House writeups"
- how2heap glibc-2.39 branch: <https://github.com/shellphish/how2heap>
