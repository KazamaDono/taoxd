# Chapter 17 — Use-After-Free and Type Confusion in C++

Companion source for Chapter 17 of *Modern Exploit Development*. **Lab use only** — see [`../ETHICS.md`](../ETHICS.md).

> ⚠️ **`.ci-skip`ped in the open-source cut.** The chapter text, the two illustrative C++ targets, and the exploit solve script are here. A wired-up Makefile with a green `make test` (which requires disabling ASLR and specific stack alignment tricks against a hardcoded reclaim address) is omitted from this initial cut. Readers rebuild it locally with:
>
> ```bash
> g++ -O0 -no-pie -fno-stack-protector -o build/uaf_vtable src/uaf_vtable.cc
> g++ -O0 -no-pie -fno-stack-protector -o build/type_confusion src/type_confusion.cc
> setarch -R python3 exploit/uaf_solve.py
> ```
>
> Both targets are self-contained teaching artifacts.

## Contents

| Path | What it is |
|------|------------|
| `src/uaf_vtable.cc` | Minimal C++ program with a UAF over an object with a virtual method — canonical vtable-hijack shape |
| `src/type_confusion.cc` | Two related classes; a `static_cast` bug reinterprets one as the other |
| `exploit/uaf_solve.py` | pwntools solver: free, reclaim with a fake vtable, trigger the virtual call to `win()` |
| `.ci-skip` | Marks this chapter as not exercised by the Linux CI (reason inside) |

## Why this chapter matters today

UAF and type confusion are the dominant bug classes in real, high-value C++ software — browsers, PDF readers, media parsers, IPC-heavy daemons. Even when the target is protected by Clang CFI or Windows XFG (see Chapter 12), a well-shaped C++ UAF gives you a *type-legal* forward-edge target in the CFI/XFG sense, so control transfers succeed and you land wherever the attacker-controlled object's vtable pointer aims — provided the target address is inside the module's CFI bitmap. The chapter walks the interaction between these bug classes and modern CFI in detail.

## References

- Google Project Zero writeups of Chrome browser UAFs (indexed at <https://googleprojectzero.blogspot.com/>)
- CFI/XFG references in Chapter 12's *Going Deeper*
