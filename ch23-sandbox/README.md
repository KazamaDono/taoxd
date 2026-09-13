# Chapter 23 — companion code

Deliberately vulnerable teaching artifacts and a working escape for
Chapter 23 of *Modern Exploit Development*. **Lab use only** — see
`../ETHICS.md`.

## Build & run

```bash
# from the repo root, inside the pinned lab image
make -C ch23-sandbox test
```

`make test` compiles the broker, the renderer launcher, and the
exploit; sets up `/tmp/lab-sandbox/{assets,helpers}/`; runs the
end-to-end escape; and passes only when the witness file
`/tmp/lab-sandbox/ESCAPED` appears with the unique marker
`uid=`. That is the CI oracle for this chapter.

## Contents

| Path | What it is |
|------|------------|
| `seccomp_policy.c` / `.h` | Renderer's seccomp-bpf filter (listing **lst-seccomp**). Arch check, minimal allow-list, `NO_NEW_PRIVS` + `PR_SET_SECCOMP`. |
| `broker.c` | Deliberately vulnerable broker (listing **lst-broker** excerpt). `OPEN_ASSET` prefix-check bug + `RUN_HELPER` strict-name check. |
| `renderer.c` | Launcher: creates the `socketpair`, forks the broker, execs the exploit binary at `argv[1]` with the broker fd at `fd 3`. |
| `exploit.c` | Post-RCE payload (listing **lst-exploit**). Installs the seccomp jail on itself, then chains `OPEN_ASSET` traversal + `write` + `RUN_HELPER`. |
| `ipc.c` / `ipc.h` | `send_msg` / `recv_msg` / `send_fd` / `recv_fd` around `sendmsg`/`recvmsg` with `SCM_RIGHTS`. |
| `Makefile` | Build + `make test`. Every mitigation flag is on and commented. |
| `test.sh` | Human-friendly wrapper around `make clean && make test`. |

## Expected `make test` output

```
[+] launching renderer -> broker -> exploit
[+] checking witness file /tmp/lab-sandbox/ESCAPED
PASS: ch23 sandbox escape reproduced
---- /tmp/lab-sandbox/ESCAPED ----
uid=<uid>(<user>) gid=<gid>(<group>) groups=...
```

## Safe-fix walkthrough (Exercise 3)

Replace the prefix check in `handle_open_asset` with a canonicalising
one:

```c
char resolved[PATH_MAX];
if (!realpath(path, resolved)) { send_status(cli, errno); return; }
if (strncmp(resolved, ASSETS_DIR, strlen(ASSETS_DIR)) != 0) {
    send_status(cli, EACCES); return;
}
int fd = open(resolved, O_RDONLY | O_NOFOLLOW);
```

Two changes matter equally: `realpath` collapses `..` and symlinks, so
`/tmp/lab-sandbox/assets/../helpers/pwn` becomes
`/tmp/lab-sandbox/helpers/pwn` before the comparison; and `O_RDONLY |
O_NOFOLLOW` narrows the returned handle so the renderer cannot use the
descriptor to write executable payloads even if a future bug re-opens
the traversal. Re-run `make test` and the exploit fails with `EACCES`
— which is the whole exercise.

## Notes for CI

- Builds cleanly with **gcc 13** and **clang 18** on **Ubuntu 24.04
  (glibc 2.39)**; no extra packages beyond the pinned lab image.
- Deterministic: `make test` cleans `/tmp/lab-sandbox/` first and
  greps for the `uid=` marker; no dependency on ASLR state.
- Uses only standard Linux syscalls and BPF headers shipped with
  `linux-libc-dev`.
