# ch14-heap-internals/playground — instrumented allocator playground
#
# Mitigation posture:
#   -O0          : preserve the source-to-asm mapping for gdb
#   -g           : debug info
#   -no-pie      : fixed .text base so `dump`/`leak` produce comparable
#                  addresses across runs under `setarch -R`
#   (defaults)   : canary + Full RELRO + NX left ON --- this is an
#                  *observation* tool; nothing here is exploited.
# Target glibc : 2.39 (Ubuntu 24.04).

CC       ?= gcc
CFLAGS   ?= -O0 -g -no-pie -Wall
BIN      := playground

.PHONY: all test clean
all: $(BIN)

$(BIN): playground.c
	$(CC) $(CFLAGS) $< -o $@

# make test: drive the CLI through script.in under ASLR-off (setarch -R) and
# assert the invariants the book claims:
#   * alloc echoes contain the expected user pointers
#   * dump shows PREV_INUSE ("P") on a live chunk and "in-use" neighbor
#   * tcache reports count=3 for the 0x40 bin (size 0x30 -> chunk 0x40 -> bin 2)
#   * leak on the tail (index 0) equals the chunk's own address >> 12
#     (the tail-sentinel property proven in Listing 14-3)
# Exits 0 only on all assertions passing.
test: $(BIN)
	@set -e; \
	if command -v setarch >/dev/null 2>&1; then RUN="setarch $$(uname -m) -R"; else RUN=""; fi; \
	out=$$($$RUN ./$(BIN) < script.in); \
	echo "$$out"; \
	echo "--- assertions ---"; \
	echo "$$out" | grep -Eq '^alloc\[0\] size=0x30 user=0x[0-9a-f]+' \
	  && echo "OK alloc[0]" || { echo "FAIL alloc[0]"; exit 1; }; \
	echo "$$out" | grep -Eq '^alloc\[3\] size=0x30 user=0x[0-9a-f]+' \
	  && echo "OK alloc[3]" || { echo "FAIL alloc[3]"; exit 1; }; \
	echo "$$out" | grep -Eq 'flags:.*P.*\)' \
	  && echo "OK PREV_INUSE flag on live chunk" || { echo "FAIL P flag"; exit 1; }; \
	echo "$$out" | grep -Eq 'reflects US: in-use' \
	  && echo "OK neighbor P bit reflects in-use" || { echo "FAIL neighbor P"; exit 1; }; \
	echo "$$out" | grep -Eq '^  bin\[ ?2\] chunk_sz=0x40  count=3' \
	  && echo "OK tcache bin[2] count=3" || { echo "FAIL tcache count"; exit 1; }; \
	tail_user=$$(echo "$$out" | awk '/^alloc\[0\]/ {print $$3}' | sed 's/user=//'); \
	tail_raw=$$(echo "$$out" | awk '/^leak\[0\]/ {print $$3}' | sed 's/raw_fd=0x//'); \
	tail_reveal=$$(echo "$$out" | awk '/^leak\[0\]/ {print $$4}' | sed 's/reveal=0x//'); \
	expected=$$(printf '%016x\n' $$(( $$tail_user >> 12 ))); \
	echo "  tail_user=$$tail_user  raw_fd=0x$$tail_raw  reveal=0x$$tail_reveal  expected raw=0x$$expected"; \
	[ "$$(echo $$tail_raw | sed 's/^0*//')" = "$$(echo $$expected | sed 's/^0*//')" ] \
	  && echo "OK raw_fd(tail) == user>>12 (safe-linking tail sentinel)" \
	  || { echo "FAIL tail raw != user>>12"; exit 1; }; \
	[ "$$(echo $$tail_reveal | sed 's/^0*//')" = "" ] \
	  && echo "OK reveal(tail) == 0 (NULL)" \
	  || { echo "FAIL tail reveal != 0 (got $$tail_reveal)"; exit 1; }; \
	echo "PASS ch14-playground"

clean:
	rm -f $(BIN)
