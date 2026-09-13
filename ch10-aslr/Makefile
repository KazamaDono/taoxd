# ch10-aslr -- chapter-level Makefile.
#
# `make test` (the CI gate) delegates to the leak-server lab.  The
# partial-overwrite lab has its own `make -C partial test` runnable
# locally; it is not gated by CI because the layout check is toolchain-
# dependent and its solve is exercise material (see chapter Exercise 4).

.PHONY: all test clean leak-server partial all-test

all: leak-server partial

leak-server:
	$(MAKE) -C leak-server all

partial:
	$(MAKE) -C partial all

test:
	$(MAKE) -C leak-server test

# Optional target that also runs the partial lab.  Not wired into CI.
all-test:
	$(MAKE) -C leak-server test
	$(MAKE) -C partial test

clean:
	$(MAKE) -C leak-server clean
	$(MAKE) -C partial clean
