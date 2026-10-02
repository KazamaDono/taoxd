# Modern Exploit Development — reproducible lab image.
#
# Pinned to a specific Ubuntu 24.04 LTS *digest* (not just the floating
# `24.04` tag) so the base rootfs — glibc, gcc, gdb, binutils — is frozen and
# every machine builds from the identical starting point. The exploit tooling
# (pwntools/ROPgadget/ropper/one_gadget/…) is pinned exactly in
# lab/requirements.txt and in the `gem install` line below, because it is
# unpinned toolchain drift — newer pwntools/ROPgadget/ropper changing APIs and
# output formats — that broke chapter tests (issue #1), not the C compiler.
#
# NOTE on apt versions vs. Figure 2-1: the 24.04 archive only ever carries the
# current point release of each package (e.g. gcc 13.3.0, gdb 15.1 today), so
# Figure 2-1's exact apt point-releases cannot be reinstalled from the live
# archive once superseded. That is fine: every chapter exploit *discovers* its
# offsets at runtime rather than hard-coding them, so it stays correct across
# 24.04 compiler/debugger point releases. Reproducibility that matters for the
# tests comes from the pinned exploit tooling below.
#
# To refresh the base digest intentionally: `docker pull ubuntu:24.04`, copy the
# printed digest here, rebuild, and run `make doctor && make test-all`.
FROM ubuntu:24.04@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC \
    LANG=C.UTF-8 \
    PIP_BREAK_SYSTEM_PACKAGES=1 \
    PWNLIB_NOTERM=1
# PWNLIB_NOTERM=1: never let pwntools enter interactive "terminal mode" inside
# the lab. With a real TTY it otherwise seizes the terminal and the
# format-string / brute solves hang on cursor-position queries (CI has no TTY,
# so this never bit CI). It does not change exploit behavior.

# ---- Core toolchain (x86-64) + AArch64 cross + emulation --------------------
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential gcc g++ gdb gdb-multiarch gdbserver make cmake nasm \
        clang lld llvm libclang-rt-dev \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        qemu-user qemu-user-static qemu-system-arm qemu-system-misc \
        python3 python3-pip python3-venv python3-dev \
        ruby ruby-dev \
        git curl wget file xxd bsdmainutils binutils elfutils \
        ltrace strace patchelf \
        vim less netcat-openbsd socat unzip zip ca-certificates tmux \
        libc6-dbg libssl-dev zlib1g-dev; \
    rm -rf /var/lib/apt/lists/*

# ---- Exploitation toolkit (Python + Ruby), version-locked ------------------
# Python tools come from lab/requirements.txt (exact pins, matching Figure 2-1).
# The gem tools are pinned here for the same reason: one_gadget's output format
# is parsed by chapter exploits, so its version must not drift.
COPY lab/requirements.txt /tmp/requirements.txt
RUN set -eux; \
    pip3 install --no-cache-dir -r /tmp/requirements.txt; \
    gem install --no-document one_gadget -v 1.9.0; \
    gem install --no-document seccomp-tools -v 1.6.1

# ---- pwndbg (primary debugger front-end) -----------------------------------
# Pinned to a release tag (not a floating `main`) so the debugger front-end is
# reproducible too; bump deliberately like the other pins.
RUN set -eux; \
    git clone --depth 1 --branch 2026.09.15 https://github.com/pwndbg/pwndbg /opt/pwndbg; \
    cd /opt/pwndbg; ./setup.sh || true; \
    echo "source /opt/pwndbg/gdbinit.py" > /root/.gdbinit

# ---- Vulnerability-research tooling (best-effort heavy layer) ---------------
# AFL++ and honggfuzz from the distro; angr/semgrep from PyPI. libFuzzer ships
# with clang. CodeQL/weggli are installed per-chapter where needed.
# This layer is intentionally NOT version-locked and is best-effort (`|| true`):
# none of it is exercised by the book's exploit tests (`make test-all`), so its
# drift cannot break reproducibility of the labs. Pin here only if a specific
# chapter starts depending on an exact angr/semgrep version.
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends afl++ honggfuzz || true; \
    pip3 install --no-cache-dir angr semgrep || true; \
    rm -rf /var/lib/apt/lists/*

# ---- Determinism helpers ----------------------------------------------------
# ASLR is disabled *inside the container* per-run via `setarch -R` in tests,
# not globally, so the host is unaffected.
WORKDIR /work
CMD ["/bin/bash"]
