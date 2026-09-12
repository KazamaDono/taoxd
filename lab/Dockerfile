# Modern Exploit Development — reproducible lab image.
# Pinned to Ubuntu 24.04 LTS so glibc/gcc/offsets are stable across machines.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=UTC \
    LANG=C.UTF-8 \
    PIP_BREAK_SYSTEM_PACKAGES=1

# ---- Core toolchain (x86-64) + AArch64 cross + emulation --------------------
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential gcc g++ gdb gdb-multiarch make cmake nasm \
        clang lld llvm libclang-rt-dev \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        qemu-user qemu-user-static qemu-system-arm qemu-system-misc \
        python3 python3-pip python3-venv python3-dev \
        ruby ruby-dev \
        git curl wget file xxd bsdmainutils binutils elfutils \
        ltrace strace ltrace patchelf \
        vim less netcat-openbsd socat unzip zip ca-certificates \
        libc6-dbg libssl-dev zlib1g-dev; \
    rm -rf /var/lib/apt/lists/*

# ---- Exploitation toolkit (Python) -----------------------------------------
RUN set -eux; \
    pip3 install --no-cache-dir \
        pwntools ROPgadget ropper capstone unicorn keystone-engine \
        pyelftools; \
    gem install --no-document one_gadget seccomp-tools

# ---- pwndbg (primary debugger front-end) -----------------------------------
RUN set -eux; \
    git clone --depth 1 https://github.com/pwndbg/pwndbg /opt/pwndbg; \
    cd /opt/pwndbg; ./setup.sh || true; \
    echo "source /opt/pwndbg/gdbinit.py" > /root/.gdbinit

# ---- Vulnerability-research tooling (best-effort heavy layer) ---------------
# AFL++ and honggfuzz from the distro; angr/semgrep from PyPI. libFuzzer ships
# with clang. CodeQL/weggli are installed per-chapter where needed.
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
