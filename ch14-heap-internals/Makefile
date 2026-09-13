# Chapter 14 top-level Makefile: fans out to safelink/ and playground/.
# CI runs `make -C ch14-heap-internals test`.

SUBDIRS := safelink playground

.PHONY: all test clean $(SUBDIRS)

all:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d all || exit 1; done

test:
	@for d in $(SUBDIRS); do \
	  echo "=== ch14: $$d test ==="; \
	  $(MAKE) -C $$d test || exit 1; \
	done
	@echo "PASS ch14-heap-internals (all subdirs)"

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
