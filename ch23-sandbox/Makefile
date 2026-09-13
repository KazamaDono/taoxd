# ch23-sandbox/Makefile — build the deliberately-vulnerable broker,
# the renderer launcher, and the exploit payload; then run the escape
# and grep for a witness file to prove it worked.
#
# Mitigation posture (documented on purpose):
#   -O2               : realistic optimisation; keeps generated syscalls
#                       close to what a release build would emit.
#   -D_FORTIFY_SOURCE=3, -fstack-protector-strong, -fPIE -pie,
#   -Wl,-z,relro,-z,now : ON for ALL binaries. This lab is not about
#                       memory corruption; the whole point of the chapter
#                       is that the escape is a LOGIC bug in the broker's
#                       path check, so we build with every conventional
#                       userland mitigation enabled and still succeed.
#   -Wall -Wextra     : normal hygiene.
#
# Nothing here disables ASLR or stack canaries. The exploit does not
# depend on a fixed address; it depends on the broker's missing
# realpath() call. That is the entire pedagogical point.

CC       ?= gcc
CFLAGS   ?= -O2 -g -std=gnu11 -Wall -Wextra \
            -D_FORTIFY_SOURCE=3 -fstack-protector-strong \
            -fPIE
LDFLAGS  ?= -pie -Wl,-z,relro,-z,now

LAB_ROOT := /tmp/lab-sandbox

BINS := renderer exploit

.PHONY: all clean test dirs

all: $(BINS)

renderer: renderer.c broker.c ipc.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

exploit: exploit.c ipc.c seccomp_policy.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

dirs:
	mkdir -p $(LAB_ROOT)/assets $(LAB_ROOT)/helpers
	# Drop a benign asset so the whitelisted path exists.
	printf 'benign\n' > $(LAB_ROOT)/assets/hello.txt
	# Clear any previous witness so `test` is deterministic.
	rm -f $(LAB_ROOT)/ESCAPED $(LAB_ROOT)/PWNED $(LAB_ROOT)/helpers/pwn

# `make test` builds everything, runs the escape end-to-end, and
# succeeds ONLY if the witness file appears with the unique marker.
test: all dirs
	@echo "[+] launching renderer -> broker -> exploit"
	./renderer ./exploit || true
	@echo "[+] checking witness file $(LAB_ROOT)/ESCAPED"
	@test -f $(LAB_ROOT)/ESCAPED || { \
	    echo "FAIL: witness file missing (escape did not run)"; \
	    exit 1; }
	@grep -q 'uid=' $(LAB_ROOT)/ESCAPED || { \
	    echo "FAIL: witness file lacks 'uid=' marker"; \
	    cat $(LAB_ROOT)/ESCAPED; exit 1; }
	@test -f $(LAB_ROOT)/PWNED || { \
	    echo "FAIL: PWNED marker missing"; exit 1; }
	@echo "PASS: ch23 sandbox escape reproduced"
	@echo "---- $(LAB_ROOT)/ESCAPED ----"
	@cat $(LAB_ROOT)/ESCAPED

clean:
	rm -f $(BINS) *.o
	rm -rf $(LAB_ROOT)
