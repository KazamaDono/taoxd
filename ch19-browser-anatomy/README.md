# Chapter 19 — companion code

Deliberately vulnerable teaching artifacts and working exploits for Chapter 19 of *Modern Exploit Development*. **Lab use only** — see `../ETHICS.md`.

Chapter 19 is architectural: it builds the mental model the rest of Part V exploits. The runnable code here therefore has two roles.

1. A **Mojo-shaped toy broker** (`toy-ipc/`) that reproduces, in about a hundred lines of Python, the trust asymmetry a real Chromium broker enforces. This is what CI exercises on every push.
2. Build scripts (`scripts/`) that pin the `d8` used across Chapters 20–22, so offsets and behavior in the later chapters remain reproducible. These are heavyweight (depot_tools, ~40 min build) and are **not** run in CI.

## Build & run (inside the pinned lab image)

```bash
# from the repo root
./scripts/lab.sh                             # drop into the Ubuntu 24.04 lab container
make -C ch19-browser-anatomy test            # runs the toy-ipc broker demo end-to-end
make -C ch19-browser-anatomy fetch-d8        # (optional) fetch pinned prebuilt d8
make -C ch19-browser-anatomy build-d8        # (optional) build d8 from source at pinned tag
make -C ch19-browser-anatomy smoke           # (optional) run scripts/smoke.js against build/d8
```

`make test` seeds `/tmp/toy-ipc-lab/`, forks the broker over a `socketpair`, and drives three scripted requests. On success it prints the unique marker `TOY_IPC_ALL_OK` on the last line and exits 0.

### Expected `make test` output

```
[allowed] OK: /tmp/toy-ipc-lab/hello.txt
[denied-abs] ERR: policy: path denied
[denied-symlink] ERR: policy: path denied
TOY_IPC_ALL_OK
PASS: ch19 toy-ipc trust-boundary demo
```

The three lines above correspond to the three trust decisions the chapter dissects at ❹–❺ of Listing 19-1:

| Request                                    | Broker decision                                    | Why                                                     |
|--------------------------------------------|----------------------------------------------------|---------------------------------------------------------|
| `open("/tmp/toy-ipc-lab/hello.txt")`       | **Allow** — attaches `fd` via `SCM_RIGHTS`         | Path realpaths inside the allowlisted prefix            |
| `open("/etc/passwd")`                      | **Deny** — `policy: path denied`                   | Absolute path lies outside the allowlist                |
| `open("/tmp/toy-ipc-lab/escape-link")`     | **Deny** — `policy: path denied`                   | `realpath` resolves the symlink to `/etc/passwd` first  |

## Contents

| Path | What it is |
|------|------------|
| `README.md` | This file. |
| `Makefile` | `test`, `toy-ipc`, `fetch-d8`, `build-d8`, `smoke`, `clean`. The `test` target is the CI entry point and greps `TOY_IPC_ALL_OK`. |
| `D8_VERSION` | Pinned V8 tag consumed by both build scripts; single source of truth for Chapters 20–22 offsets. |
| `scripts/fetch-d8.sh` | Downloads a prebuilt `d8` for the pinned tag into `build/` and records a SHA-256. |
| `scripts/build-d8.sh` | Provisions `depot_tools`, checks out the pinned tag, applies lab patches under `patches/` (if any), and builds `d8` with `v8_enable_disassembler`, `v8_enable_object_print`, pointer compression, and the V8 heap sandbox enabled. |
| `scripts/smoke.js` | Verbatim Listing 19-2. Verifies `%DebugPrint`, TurboFan warm-up, and pointer-compression banner on the built `d8`. |
| `toy-ipc/broker.py` | Verbatim Listing 19-1 body: length-prefixed JSON over Unix socket, allowlist authorization, `realpath` canonicalization, `SCM_RIGHTS` fd handoff. Non-listing helpers live below the fold and are marked as such. |
| `toy-ipc/renderer.py` | Sandbox-shaped renderer client: `send_request`, `recv_response`, `open_via_broker`. Cannot call `open` itself. |
| `toy-ipc/driver.py` | Forks the broker over a `socketpair` and issues the three scripted requests; asserts outcomes; prints `TOY_IPC_ALL_OK`. |
| `toy-ipc/test_broker.py` | pytest- and script-compatible harness that runs `driver.py` and checks the three outcomes. |
| `toy-ipc/__init__.py` | Package marker so `python3 -m toy_ipc.driver` resolves via the `toy_ipc` alias the driver installs. |

## What the code demonstrates (and what it does not)

The toy is Python and holds only the *pattern* the chapter names:

* one process (the broker) is the only one that calls the dangerous syscall;
* the other process (the renderer) has to describe what it wants in a structured message;
* authorization is decided by the broker after **canonicalization**, not by the renderer;
* handles cross the boundary via an OS handle-passing primitive, not by re-opening.

It is **not** Mojo: no `.mojom` compiler, no interface routing, no shared memory, no `NodeChannel`. Chapter 30 will look at the real Mojo bindings when we fuzz them.

## Reproducing the escape from Exercise 3

Exercise 3 removes the `realpath` canonicalization at ❹ of the listing. To reproduce it locally:

```bash
sed -i 's/os.path.realpath(req.get("path", ""))/req.get("path", "")/' \
    toy-ipc/broker.py
make test          # expect FAIL: the symlink-escape request now succeeds
git checkout -- toy-ipc/broker.py
```

Do not run the broken broker outside the lab. That change is the shape of a real published sandbox-escape class.
