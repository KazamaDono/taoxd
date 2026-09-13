# Building the pinned lab kernel and initramfs

The QEMU lab boots two artifacts that live under `lab/artifacts/`:

- `bzImage` — a Linux 6.8 kernel built from the pinned tree.
- `vmlinux` — the same build, unstripped, for gdb symbols.
- `initramfs.cpio.gz` — a BusyBox rootfs that auto-`insmod`s `vuln.ko`.

Neither is committed to the repo (they are large and rebuildable). The steps
below reproduce them on the pinned Ubuntu 24.04 lab image.

## 1. Fetch and configure Linux 6.8

```bash
cd /tmp
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.8.tar.xz
tar xf linux-6.8.tar.xz
cd linux-6.8

# Start from the default x86-64 config and enable the mitigations Chapter 24
# talks about, plus what BusyBox needs.
make defconfig
scripts/config \
    --enable STACKPROTECTOR_STRONG \
    --enable RANDOMIZE_BASE \
    --enable RANDOMIZE_MEMORY \
    --enable PAGE_TABLE_ISOLATION \
    --enable HARDENED_USERCOPY \
    --enable STACKLEAK \
    --enable INIT_ON_ALLOC_DEFAULT_ON \
    --enable MODULES --enable MODULE_UNLOAD \
    --enable MISC_FILESYSTEMS \
    --enable DEVTMPFS --enable DEVTMPFS_MOUNT \
    --enable PRINTK --enable EARLY_PRINTK \
    --disable MODULE_SIG_FORCE
make olddefconfig
make -j"$(nproc)" bzImage
```

Copy the results into the repo:

```bash
mkdir -p code/ch24-kernel-intro/lab/artifacts
cp arch/x86/boot/bzImage code/ch24-kernel-intro/lab/artifacts/bzImage
cp vmlinux              code/ch24-kernel-intro/lab/artifacts/vmlinux
```

## 2. Build the vulnerable module against that tree

```bash
cd code/ch24-kernel-intro/vuln
make KDIR=/tmp/linux-6.8
```

You should get `vuln.ko`.

## 3. Build a BusyBox initramfs that auto-loads it

```bash
cd /tmp
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2 && cd busybox-1.36.1
make defconfig
scripts/config --enable STATIC
make -j"$(nproc)"

mkdir -p rootfs/{bin,sbin,proc,sys,dev,tmp,lib/modules}
cp busybox rootfs/bin/busybox
for a in sh insmod mount mknod cat echo sleep ls; do
    ln -sf /bin/busybox rootfs/bin/$a
done
cp /path/to/repo/code/ch24-kernel-intro/vuln/vuln.ko rootfs/lib/modules/
cp /path/to/repo/code/ch24-kernel-intro/exploit/trigger rootfs/bin/

cat >rootfs/init <<'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev
insmod /lib/modules/vuln.ko
echo "== vuln loaded, base:"
cat /sys/module/vuln/sections/.text
exec /bin/sh
EOF
chmod +x rootfs/init

(cd rootfs && find . | cpio -o -H newc | gzip -9) \
    > /path/to/repo/code/ch24-kernel-intro/lab/artifacts/initramfs.cpio.gz
```

## 4. Boot it

```bash
cd code/ch24-kernel-intro
./lab/run.sh
```

To attach gdb:

```bash
# terminal 1
DEBUG=1 ./lab/run.sh
# terminal 2
export MOD_BASE=$(...)   # from the serial-console line above
./lab/debug.sh
```

## Notes

- The kernel build is heavyweight (10–20 min on a laptop). CI does **not**
  build it; see `.ci-skip` at the chapter root for the reason.
- AArch64 variant: repeat step 1 with `ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-`
  and boot with `qemu-system-aarch64 -M virt -cpu max` in step 4.
