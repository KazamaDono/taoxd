# Running syzkaller against the ch24 lab driver

Prerequisites (host, Ubuntu 24.04):

- Go 1.22+ (`sudo apt install golang-go`)
- QEMU 8.2+ (`qemu-system-x86_64`)
- The Chapter 24 kernel already built (`code/ch24-kernel-intro/`), producing
  `arch/x86/boot/bzImage`. Rebuild with `CONFIG_KASAN=y`, `CONFIG_KCOV=y`,
  and `CONFIG_DEBUG_INFO=y` — syzkaller needs KCOV for coverage and KASAN
  turns the driver's kernel-stack overflow into a loud report instead of
  silent corruption.
- A bootable rootfs image with an SSH server. The ch24 lab boots from a
  BusyBox initramfs (see `ch24-kernel-intro/lab/run.sh`) which does not
  ship an SSH server, so it is not directly reusable here — syzkaller's
  `qemu` VM type SSHes into each disposable VM. Build one with
  `syzkaller/tools/create-image.sh` pinned to Noble (Ubuntu 24.04) to match
  `bible/tech-stack.md`, and drop the resulting `noble.img` /
  `noble.id_rsa` into `./image/` alongside `config.cfg`.

## Build syzkaller with the ch24 description

```bash
git clone https://github.com/google/syzkaller
cd syzkaller
cp ../../ch30-fuzzing-2/syzkaller/ch24_lkm.txt sys/linux/ch24_lkm.txt
make extract TARGETOS=linux SOURCEDIR=/path/to/linux   # extract kernel consts
make generate                                          # regenerate syzlang bindings
make TARGETOS=linux TARGETARCH=amd64
```

`make generate` compiles the new `.txt` into Go stubs the fuzzer links in.
If the description has an error you'll see it here before you burn CPU on a
broken campaign. In particular, an ioctl `cmd` constant that does not
match the running kernel's `_IOC` layout will cause the driver to return
`-ENOTTY` and no coverage inside `vuln_ioctl` — see the arithmetic in
`ch24_lkm.txt`.

## Run

```bash
mkdir -p image && cp /path/to/noble.img image/ && cp /path/to/noble.id_rsa image/
cp ../../ch30-fuzzing-2/syzkaller/config.cfg .
./bin/syz-manager -config=config.cfg
```

Open http://127.0.0.1:56741 in a browser. The dashboard shows:

- **Coverage**: PC bitmap over the kernel; filter to `vuln` to see the
  driver's `.text`. If coverage of `vuln_write` and `vuln_ioctl` is zero
  after ten minutes, the description is wrong — start there.
- **Crashes**: minimized reproducer as a syz program plus a translated C
  program. The kernel-stack overflow planted in the driver typically
  appears within minutes as `openat$vuln` followed by a `write$vuln` with
  a long `buf`, tripping a KASAN or stack-guard panic.

## Reproduce a single program deterministically

```bash
./bin/syz-execprog -executor=./bin/syz-executor -repeat=1 -threaded=0 \
    ./workdir-ch24/crashes/<hash>/prog0
```

`-repeat=1 -threaded=0` disables the collision-mode replay so you get a
one-shot execution with the exact call sequence. If a crash does not
reproduce, that's usually the entropy problem from the snapshot warning:
any driver that touches `get_random_bytes` or a jiffies-based path needs
its non-determinism stubbed before you retry.

## Coverage minimization for a bug report

```bash
./bin/syz-prog2c ./workdir-ch24/crashes/<hash>/prog0 > repro.c
./bin/syz-minimize -config=config.cfg -crash=./workdir-ch24/crashes/<hash>/
```

The minimized C reproducer plus the syzlang program are what upstream
maintainers want to see attached to a report — the same shape as the
crashes on the public `syzbot` dashboard.
