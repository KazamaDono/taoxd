# Kbuild wrapper — builds vuln.ko against the running kernel headers.
#
# Mitigation stance for the vulnerable module (DELIBERATE):
#   * We do NOT pass -fno-stack-protector; on Ubuntu 6.8 the kernel is built
#     with CONFIG_STACKPROTECTOR_STRONG=y, so vuln.ko inherits a canary. That
#     is what turns the write() overflow into __stack_chk_fail (see ch24 §Building the lab).
#   * We do NOT disable SMAP/SMEP or hardened usercopy at compile time — those
#     are kernel-wide policies enforced at runtime by the pinned lab kernel.
#   * -Wframe-larger-than is left at the kernel default so the 64-byte kbuf on
#     the stack does not trigger a build warning we would otherwise want.
#
# KDIR points at the kernel build tree. Override on the command line, e.g.
#   make KDIR=/lib/modules/6.8.0-45-generic/build
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m := vuln.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
