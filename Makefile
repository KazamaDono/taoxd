# Top-level convenience targets. Real work lives in each chNN-*/Makefile.
.PHONY: help build-all test-all doctor clean
help:
	@echo "targets:"
	@echo "  make build-all   build every chapter that has a Makefile"
	@echo "  make test-all    run every chapter's exploit test (CI does this)"
	@echo "  make doctor      check the lab toolchain"
	@echo "  make clean       clean every chapter"
build-all:
	@bash scripts/build-all.sh
test-all:
	@bash scripts/test-all.sh
doctor:
	@bash scripts/doctor.sh
clean:
	@for d in ch*/; do [ -f "$$d/Makefile" ] && $(MAKE) -C "$$d" clean 2>/dev/null || true; done
