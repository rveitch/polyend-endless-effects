# Compatibility entry point. Effect sources and SDK now live at repository root.
.DEFAULT_GOAL := all
TOOLCHAIN ?= arm-none-eabi-
HOST_CXX ?= c++
PYTHON ?= python3
EFFECT ?= junoChorus
PATCH_NAME ?= patch
BUILD_DIR ?= build
PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUTPUT_DIR := $(abspath $(BUILD_DIR))
export TOOLCHAIN HOST_CXX
all:
	$(PYTHON) "$(PROJECT_ROOT)/scripts/effects.py" build --effect "$(EFFECT)" --build-dir "$(OUTPUT_DIR)" --patch-name "$(PATCH_NAME)" --extra-flags="$(CUSTOM_COMPILER_OPTIONS)"
test:
	$(PYTHON) "$(PROJECT_ROOT)/scripts/effects.py" test --effect "$(EFFECT)" --build-dir "$(OUTPUT_DIR)"
clean:
	@echo 'Saved legacy artifacts are preserved. Use the root clean target for managed builds.'
.PHONY: all test clean
