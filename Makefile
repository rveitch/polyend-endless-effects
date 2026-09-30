.DEFAULT_GOAL := build
PYTHON ?= python3
EFFECT ?= junoChorus
TOOLCHAIN ?= arm-none-eabi-
HOST_CXX ?= c++
export TOOLCHAIN HOST_CXX
OPTIONS = --effect "$(EFFECT)" $(if $(BUILD_DIR),--build-dir "$(BUILD_DIR)") $(if $(PATCH_NAME),--patch-name "$(PATCH_NAME)") $(if $(CUSTOM_COMPILER_OPTIONS),--extra-flags="$(CUSTOM_COMPILER_OPTIONS)")
build all:
	$(PYTHON) scripts/effects.py build $(OPTIONS)
list:
	$(PYTHON) scripts/effects.py list
test:
	$(PYTHON) scripts/effects.py test $(OPTIONS)
test-tools:
	$(PYTHON) -m unittest discover -s tests -p 'test*.py' -v
check:
	$(PYTHON) scripts/effects.py check $(OPTIONS)
clean:
	$(PYTHON) scripts/effects.py clean $(OPTIONS)
.PHONY: build all list test test-tools check clean
