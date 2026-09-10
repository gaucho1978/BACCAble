.DEFAULT_GOAL := all
FLAVOR ?= CAN
VALID_FLAVORS := C1 C2 BH CAN
ifeq ($(filter $(FLAVOR),$(VALID_FLAVORS)),)
$(error FLAVOR must be one of $(VALID_FLAVORS))
endif

TOOLCHAIN ?= arm-none-eabi-
CC := $(TOOLCHAIN)gcc
OBJCOPY := $(TOOLCHAIN)objcopy
SIZE := $(TOOLCHAIN)size
BUILD_DIR := build/$(FLAVOR)
VERSION ?= $(shell git describe --always --dirty --tags)
TARGET := $(BUILD_DIR)/baccable-$(FLAVOR)
APP_DIRS := app diagnostics features platform protocol settings state storage transport vehicle
HAL_DIR := Drivers/STM32F0xx_HAL_Driver
CMSIS_DIR := Drivers/CMSIS
USB_DIR := Middlewares/ST/STM32_USB_Device_Library
INCLUDE_DIRS := . Core/Inc platform/stm32 third_party/fatfs third_party/printf \
 $(HAL_DIR)/Inc $(CMSIS_DIR)/Include $(CMSIS_DIR)/Device/ST/STM32F0xx/Include \
 $(USB_DIR)/Core/Inc $(USB_DIR)/Class/CDC/Inc $(USB_DIR)/Class/MSC/Inc \
 USB_DEVICE/App USB_DEVICE/Target
CPPFLAGS += $(addprefix -I,$(INCLUDE_DIRS)) -DUSE_HAL_DRIVER -DSTM32F072xB \
 -DRELEASE_FLAVOR=$(FLAVOR)_FLAVOR -D$(FLAVOR)_FLAVOR=1 \
 -DGIT_VERSION=\"$(VERSION)\" -DINTERNAL_OSCILLATOR $(EXTRA_CPPFLAGS)
CPU_FLAGS := -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
CFLAGS += $(CPU_FLAGS) -Os -g3 -std=gnu11 -Wall -Wextra \
 -Wno-unused-parameter -Werror=implicit-function-declaration \
 -flto -ffunction-sections -fdata-sections -fstack-usage -MMD -MP
LDFLAGS += $(CPU_FLAGS) -flto --specs=nano.specs --specs=nosys.specs \
 -TSTM32F072C8TX_FLASH.ld -Wl,--gc-sections,-Map=$(TARGET).map
LDLIBS := -Wl,--start-group -lc -lm -Wl,--end-group
SOURCES := $(sort $(shell find $(APP_DIRS) third_party USB_DEVICE -name '*.c')) \
 $(wildcard $(HAL_DIR)/Src/*.c) $(wildcard $(USB_DIR)/Core/Src/*.c) \
 $(wildcard $(USB_DIR)/Class/CDC/Src/*.c) $(wildcard $(USB_DIR)/Class/MSC/Src/*.c)
OBJECTS := $(addprefix $(BUILD_DIR)/,$(SOURCES:.c=.o))
OBJECTS += $(BUILD_DIR)/startup.o

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

# Flags and headers are inputs for application and vendor objects.
$(BUILD_DIR)/flags: FORCE
	@mkdir -p $(@D)
	@echo '$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)' > $@.tmp
	@cmp -s $@.tmp $@ || cp $@.tmp $@
	@rm -f $@.tmp

$(BUILD_DIR)/%.o: %.c $(BUILD_DIR)/flags
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.o: Core/Startup/startup_stm32f072c8tx.s $(BUILD_DIR)/flags
	$(CC) $(CPU_FLAGS) -c $< -o $@

$(TARGET).elf: $(OBJECTS) STM32F072C8TX_FLASH.ld
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD_DIR)

test:
	$(MAKE) -C ../../tests test

lint:
	cppcheck --error-exitcode=1 --std=c11 --inline-suppr \
	 --suppress=missingIncludeSystem --suppress=nullPointerOutOfMemory \
	 --suppress=nullPointer:Drivers/CMSIS/Include/core_cm0.h \
	 -D__GNUC__=10 -D__GNUC_MINOR__=3 $(CPPFLAGS) $(APP_DIRS) USB_DEVICE

flash: all
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000:leave -D $(TARGET).bin

-include $(OBJECTS:.o=.d)
.PHONY: all clean test lint flash FORCE
