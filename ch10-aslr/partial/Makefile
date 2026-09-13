# ch10-aslr/partial -- one-byte partial-overwrite lab.
#
# Mitigation flags:
#
#   -fno-stack-protector : canary OFF; the read() has to reach past the
#                          buffer without smashing a cookie.
#   -no-pie              : PIE OFF; gives `win` and `default_cb` fixed
#                          absolute addresses so the "same 256-byte window"
#                          layout requirement is checkable at link time.
#                          The technique is *identical* under PIE; only
#                          the base is randomised, and the low 12 bits of
#                          any code address are frozen anyway.
#   -O0 -fno-inline      : keep small functions distinct so their
#                          addresses are individually meaningful.
#   -Wl,-z,relro,-z,now  : Full RELRO ON (default best practice).
#
# CI note: this lab is intentionally not gated by the chapter-level
# `make test` (the top Makefile only runs the leak-server test).  Run
# `make -C ch10-aslr/partial test` manually or via `make -C ch10-aslr all-test`.

CC       ?= gcc
CFLAGS   ?= -O0 -Wall -Wno-unused-result -fno-inline \
            -fno-stack-protector -no-pie -D_GNU_SOURCE
LDFLAGS  ?= -Wl,-z,relro,-z,now -Wl,-z,noexecstack
PYTHON   ?= python3

.PHONY: all test clean layout check-tools

all: vuln

vuln: vuln.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@$(MAKE) --no-print-directory layout

# Post-link check: `win` and `default_cb` MUST live in the same
# 256-byte window so a one-byte partial overwrite of `default_cb`'s
# low byte can name `win`.  If it fails, the toolchain reordered
# something; adjust vuln.c or add an `-Wl,--section-ordering-file`.
layout: vuln
	@nm vuln | awk '/ T (win|default_cb)$$/ {print $$3, strtonum("0x"$$1)}' \
	   | sort > .layout.txt
	@awk 'NR==1{w=$$2}NR==2{d=$$2} \
	     END { \
	       if (int(w/256)!=int(d/256)) { \
	         printf "layout FAIL: win=%#x default_cb=%#x are in different 256-byte windows\n", w, d; \
	         exit 1; \
	       } else { \
	         printf "layout OK: win=%#x default_cb=%#x share window %#x\n", w, d, int(w/256)*256; \
	       } \
	     }' .layout.txt

check-tools:
	@$(PYTHON) -c "import pwn" 2>/dev/null || { \
	    echo "[test] installing pwntools into user site..."; \
	    $(PYTHON) -m pip install --quiet --user "pwntools>=4.12,<5" \
	        || { echo "[test] pip install failed"; exit 1; }; \
	}

test: vuln check-tools
	$(PYTHON) solve.py

clean:
	rm -f vuln .layout.txt
