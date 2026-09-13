# ch02-lab/Makefile — build the smoke target for x86-64 and AArch64.
CC    ?= gcc
CROSS ?= aarch64-linux-gnu-gcc

# DELIBERATELY insecure — smoke test only; Chapter 5 re-enables all of these.
#   -fno-stack-protector : no canary, so the overflow reaches the return address
#   -no-pie              : fixed load address, so win() has a stable address
#   -g -O0               : clean debugging;  -w : silence the intended-bug warnings
TEACH := -fno-stack-protector -no-pie -g -O0 -w

all: hello_overflow hello_overflow_arm64

hello_overflow: hello_overflow.c
	$(CC) $(TEACH) -o $@ $<

hello_overflow_arm64: hello_overflow.c
	$(CROSS) -static $(TEACH) -o $@ $<

test: all
	python3 solve.py amd64
	python3 solve.py aarch64

clean:
	rm -f hello_overflow hello_overflow_arm64 core *.core
.PHONY: all test clean
