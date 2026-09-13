# ch14-heap-internals/safelink — safe-linking demystifier
#
# Mitigation posture (see book Ch 11 for the taxonomy):
#   -O0          : keep the source-to-asm mapping trivial for the reader
#   -g           : line-level debug info for pwndbg walks
#   -no-pie      : fix the .text base so pwndbg output is comparable run to run
#   (defaults)   : stack-canary + Full RELRO + NX from Ubuntu 24.04 gcc are LEFT ON
#                  --- we are not exploiting this program, only observing glibc.
# Target glibc : 2.39 (Ubuntu 24.04). Safe-linking is 2.32+.

CC       ?= gcc
CFLAGS   ?= -O0 -g -no-pie -Wall
BIN      := safelink

.PHONY: all test clean
all: $(BIN)

$(BIN): safelink.c
	$(CC) $(CFLAGS) $< -o $@

# make test: build, run, pipe stdout into verify.py which asserts
#   (1) c->next reveals to b, (2) b->next reveals to a,
#   (3) a->next reveals to NULL, (4) tail-leak == a>>12 (Listing 14-3).
# verify.py exits non-zero on any failure.
test: $(BIN)
	@./$(BIN) | tee /tmp/ch14_safelink.out
	@python3 verify.py < /tmp/ch14_safelink.out
	@rm -f /tmp/ch14_safelink.out

clean:
	rm -f $(BIN)
