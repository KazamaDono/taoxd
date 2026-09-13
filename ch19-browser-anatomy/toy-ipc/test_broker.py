"""CI test for the toy Mojo-shaped broker.

Runs `driver.py` as a subprocess and asserts the three scripted outcomes,
so a regression in the broker's authorization surface is caught on push.
Works both under pytest and as a plain script (`python3 test_broker.py`)
so it needs no external dependency in CI.
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
_PKG_PARENT = _HERE.parent  # ch19-browser-anatomy/


def _run_driver() -> subprocess.CompletedProcess:
    env = os.environ.copy()
    # Make `toy_ipc` (a symlink the driver creates on first run) resolvable.
    env["PYTHONPATH"] = os.pathsep.join(
        p for p in (str(_PKG_PARENT), env.get("PYTHONPATH", "")) if p
    )
    return subprocess.run(
        [sys.executable, str(_HERE / "driver.py")],
        capture_output=True,
        text=True,
        env=env,
        timeout=30,
        check=False,
    )


def test_three_outcomes():
    r = _run_driver()
    assert r.returncode == 0, (
        f"driver exited {r.returncode}\n"
        f"--- stdout ---\n{r.stdout}\n--- stderr ---\n{r.stderr}"
    )
    out = r.stdout
    assert "[allowed] OK" in out, out
    assert "[denied-abs] ERR: policy:" in out, out
    assert "[denied-symlink] ERR: policy:" in out, out
    assert out.strip().splitlines()[-1] == "TOY_IPC_ALL_OK", out


def test_marker_present():
    r = _run_driver()
    assert "TOY_IPC_ALL_OK" in r.stdout


if __name__ == "__main__":
    failures = 0
    for name, fn in list(globals().items()):
        if name.startswith("test_") and callable(fn):
            try:
                fn()
                print(f"PASS {name}")
            except AssertionError as e:
                failures += 1
                print(f"FAIL {name}: {e}")
    sys.exit(1 if failures else 0)
