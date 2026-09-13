# Chapter 13 aggregator — build+test every hw-mitigation demo.
# See subdir Makefiles for the exact mitigation flags used per demo.

.PHONY: all test clean
SUBDIRS = cet pac mte

all:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d all || exit $$?; done

test:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d test || exit $$?; done
	@echo "ch13-hwmit: all demos exercised (see per-demo OK/SKIP lines)"

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
