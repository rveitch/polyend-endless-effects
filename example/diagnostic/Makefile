# Run from example/: make -C example -f diagnostic/Makefile
PATCH_NAME := endless_passthrough
PATCH_IMPL := diagnostic/Passthrough.cpp
BUILD_DIR := build-passthrough
include Makefile

HOST_CXX ?= c++
HOST_TEST := $(BUILD_DIR)/passthrough-test

test: $(HOST_TEST)
	$(HOST_TEST)

$(HOST_TEST): diagnostic/tests/PassthroughTest.cpp diagnostic/Passthrough.cpp source/Patch.h | $(BUILD_DIR)
	$(HOST_CXX) -std=c++20 -Wall -Wextra -Werror diagnostic/tests/PassthroughTest.cpp diagnostic/Passthrough.cpp -o $@

.PHONY: test
