# Chapter 20 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 20 of *Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh            # drop into the Ubuntu 24.04 lab container
make -C ch20-js-internals test        # build the target(s) and run the exploit test
```

Every runnable lab ships a `make test` that asserts the exploit reaches its goal; CI runs it on every push.

## Contents

| Path | What it is |
|------|------------|
| _populated as this chapter's code lands_ | |
