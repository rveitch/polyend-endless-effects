# Project build layer. vendor/FxPatchSDK remains byte-for-byte upstream.
.DEFAULT_GOAL := all
TOOLCHAIN ?= arm-none-eabi-
BUILD_DIR ?= build
PATCH_NAME ?= patch
PATCH_LOAD_ADDR ?= 0x80000000
EFFECT_ID ?= junoChorus
PATCH_IMPL ?= effects/$(EFFECT_ID)/PatchImpl.cpp
SDK_DIR := vendor/FxPatchSDK
CC := $(TOOLCHAIN)gcc
CXX := $(TOOLCHAIN)g++
OBJECT_DIR := $(BUILD_DIR)/objects/$(EFFECT_ID)
COMMON_FLAGS := -Wall -Wextra -Werror -fno-builtin -fno-common -ffunction-sections -fdata-sections \
 -fsingle-precision-constant -g -mfloat-abi=hard -mthumb -fstack-usage -specs=nano.specs \
 -Wdouble-promotion -mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfp16-format=ieee -O3 -DNDEBUG
CFLAGS := $(COMMON_FLAGS) -std=c11 $(CUSTOM_COMPILER_OPTIONS)
CXXFLAGS := $(COMMON_FLAGS) -std=c++20 -includecstdint -felide-constructors -Wno-psabi -fno-exceptions -fno-rtti $(CUSTOM_COMPILER_OPTIONS)
INCLUDES := -I$(SDK_DIR)/source
SRC_C := $(wildcard $(SDK_DIR)/internal/*.c)
SRC_CPP := $(filter-out $(SDK_DIR)/source/PatchImpl.cpp,$(wildcard $(SDK_DIR)/source/*.cpp)) \
 $(wildcard $(SDK_DIR)/internal/*.cpp) $(PATCH_IMPL)
OBJ := $(patsubst %.c,$(OBJECT_DIR)/%.o,$(SRC_C)) $(patsubst %.cpp,$(OBJECT_DIR)/%.o,$(SRC_CPP))
LDFLAGS := -nostartfiles --specs=nosys.specs -Wl,-gc-sections -T $(SDK_DIR)/internal/patch_imx.ld \
 -Wl,--defsym=PATCH_LOAD_ADDR=$(PATCH_LOAD_ADDR) -Wl,--defsym=end=__patch_bss_end
LIBS := -Wl,--start-group -lstdc++ -lc -lm -lgcc -Wl,--end-group
PATCH_ELF := $(BUILD_DIR)/$(PATCH_NAME).elf
CONFIG_FILE := $(OBJECT_DIR)/buildConfig.json
all: $(PATCH_BIN)
$(OBJECT_DIR)/%.o: %.c $(CONFIG_FILE)
	@mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
$(OBJECT_DIR)/%.o: %.cpp $(CONFIG_FILE)
	@mkdir -p "$(dir $@)"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
$(PATCH_ELF): $(OBJ) $(SDK_DIR)/internal/patch_imx.ld $(CONFIG_FILE) FORCE
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS) $(LIBS)
$(PATCH_BIN): $(PATCH_ELF)
	$(TOOLCHAIN)objcopy -O binary $< $@
-include $(OBJ:.o=.d)
.PHONY: all FORCE
FORCE:
