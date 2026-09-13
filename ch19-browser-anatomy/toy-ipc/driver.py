"""Scripted three-request demo of the toy Mojo-shaped broker.

Forks a broker child over a Unix `socketpair`; the parent plays the role of
the sandboxed renderer and issues:

  1. an *allowed* Open on a lab file under the allowlist,
  2. a *denied* Open on /etc/passwd (outside the allowlist),
  3. a *symlink-escape* Open on a lab-path symlink pointing at /etc/passwd,
     which the broker rejects because `os.path.realpath` canonicalizes
     before the prefix check (❹ in the book listing).

On success the driver prints the unique marker `TOY_IPC_ALL_OK` on its last
line; the Makefile `test` target greps for that marker and exits non-zero if
any of the three outcomes drifts.
"""
from __future__ import annotations

import os
import socket
import sys
import tempfile
import time
from pathlib import Path

# Make sibling module importable whether we're run as `python3 driver.py`
# or as `python3 -m toy_ipc.driver`.
_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import renderer  # noqa: E402  (path set above)


LAB_ROOT = "/tmp/toy-ipc-lab"
LAB_FILE = f"{LAB_ROOT}/hello.txt"
LAB_LINK = f"{LAB_ROOT}/escape-link"
DENIED_PATH = "/etc/passwd"


def _seed_lab() -> None:
    """Create the allowlisted directory, a real file inside it, and a
    symlink inside it that points at /etc/passwd. The symlink is the
    time-of-check/time-of-use bait: without realpath the broker would
    open /etc/passwd because the requested path *starts* with the
    allowlisted prefix."""
    os.makedirs(LAB_ROOT, exist_ok=True)
    with open(LAB_FILE, "w") as f:
        f.write("hello from the lab\n")
    try:
        os.remove(LAB_LINK)
    except FileNotFoundError:
        pass
    os.symlink(DENIED_PATH, LAB_LINK)


def _spawn_broker(broker_end: socket.socket) -> int:
    """Fork the broker child, hand it the peer end of the socketpair on
    argv[1]. The child never returns; the parent gets its pid."""
    pid = os.fork()
    if pid == 0:
        # Child: close the parent's end, dup the broker end onto a known
        # fd (avoid clashing with 0/1/2), exec the broker module.
        os.set_inheritable(broker_end.fileno(), True)
        broker_fd = broker_end.fileno()
        os.execvp(
            sys.executable,
            [sys.executable, "-m", "toy_ipc.broker", str(broker_fd)],
        )
    return pid


def _describe(label: str, resp: dict, fd) -> str:
    marker = "OK" if resp and resp.get("ok") else "ERR"
    detail = resp.get("path") if resp and resp.get("ok") else (
        resp.get("error") if resp else "no-response")
    return f"[{label}] {marker}: {detail}"


def main() -> int:
    _seed_lab()

    # Package-import path: make `toy_ipc` resolve to our on-disk `toy-ipc/`.
    pkg_parent = _HERE.parent
    pkg_alias = pkg_parent / "toy_ipc"
    created_alias = False
    if not pkg_alias.exists():
        # symlink toy_ipc -> toy-ipc so `-m toy_ipc.broker` works.
        try:
            os.symlink(_HERE.name, pkg_alias)
            created_alias = True
        except OSError:
            # Fall back to sys.path shimming if symlink fails.
            pass
    env_pypath = os.environ.get("PYTHONPATH", "")
    os.environ["PYTHONPATH"] = os.pathsep.join(
        p for p in (str(pkg_parent), env_pypath) if p
    )

    parent_sock, broker_sock = socket.socketpair(
        socket.AF_UNIX, socket.SOCK_STREAM
    )
    broker_pid = _spawn_broker(broker_sock)
    broker_sock.close()

    outcomes: list[tuple[str, dict, object]] = []
    try:
        # (1) Allowed read: real file under the allowlisted prefix.
        r1, fd1 = renderer.open_via_broker(parent_sock, LAB_FILE)
        outcomes.append(("allowed", r1, fd1))
        if fd1 is not None:
            # Prove the descriptor is genuinely usable in the renderer even
            # though the renderer never called open() itself.
            with os.fdopen(fd1, "rb", closefd=True) as f:
                _ = f.read(64)

        # (2) Denied read: path outside the allowlist.
        r2, fd2 = renderer.open_via_broker(parent_sock, DENIED_PATH)
        outcomes.append(("denied-abs", r2, fd2))
        if fd2 is not None:
            os.close(fd2)

        # (3) Symlink-escape: path under the allowlist that resolves out.
        r3, fd3 = renderer.open_via_broker(parent_sock, LAB_LINK)
        outcomes.append(("denied-symlink", r3, fd3))
        if fd3 is not None:
            os.close(fd3)
    finally:
        parent_sock.close()
        # Reap the broker child so the process table stays tidy.
        try:
            os.waitpid(broker_pid, 0)
        except ChildProcessError:
            pass
        if created_alias:
            try:
                os.unlink(pkg_alias)
            except OSError:
                pass

    for label, resp, fd in outcomes:
        print(_describe(label, resp, fd))

    # Assert the three scripted outcomes; drift -> non-zero exit.
    ok = (
        outcomes[0][1].get("ok") is True
        and outcomes[1][1].get("error", "").startswith("policy:")
        and outcomes[2][1].get("error", "").startswith("policy:")
    )
    if not ok:
        print("FAIL: at least one outcome drifted; see log above",
              file=sys.stderr)
        return 1

    # Unique marker the Makefile greps for.
    print("TOY_IPC_ALL_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
