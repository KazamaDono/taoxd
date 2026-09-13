# ETHICS — Chapter 24 lab

`vuln.ko` is **deliberately vulnerable** and world-writable on purpose. It
exposes:

- an unbounded `copy_from_user` in `write(2)` (kernel-stack overflow),
- an `ioctl(2)` that returns a raw kernel `.text` pointer (KASLR leak).

## Do

- Build it and load it **only** inside the QEMU VM produced by
  `lab/run.sh`. That VM has no network and no persistent storage.
- Use it to reproduce the walkthroughs in Chapter 24 and to work the
  exercises there and in Chapters 25–26.

## Do not

- `insmod vuln.ko` on any host, shared VM, container that shares a kernel
  with anything else, cloud instance, or CI runner. Any user on the box
  gets local root plus a KASLR leak while the module is loaded.
- Ship this module inside another distribution, container image, or product.
- Adapt these techniques against kernels you do not own and have not been
  authorized to test.

The blast radius when kernel exploits go wrong is a rebooted machine and
possibly lost data; the blast radius when they succeed against something
you do not own is a criminal offense. See the repository root `ETHICS.md`
and Appendix D of the book.
