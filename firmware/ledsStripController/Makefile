# STM32F0xx Makefile
# #####################################
#
# Part of the uCtools project
# uctools.github.com
#
#######################################
# user configuration:
#######################################


# SOURCES: list of sources in the user application
SOURCES = main.c can.c lowConsume.c onboardLed.c slcan.c stm32f0xx_it.c sysmem.c uart.c error.c printf.c stm32f0xx_hal_msp.c syscalls.c system_stm32f0xx.c vuMeter.c
USB_SOURCES_APP = usb_device.c usbd_cdc_if.c usbd_desc.c
USB_SOURCES_TARGET = usbd_conf.c

# Get git version and dirty flag
GIT_VERSION := $(shell git describe --abbrev=7 --dirty --always --tags)
GIT_REMOTE := $(shell git config --get remote.origin.url | sed 's/^.*github/github/')

# TARGET: name of the user application
TARGET = baccable-$(GIT_VERSION)

# BUILD_DIR: directory to place output files in
BUILD_DIR = build

# LD_SCRIPT: location of the linker script
LD_SCRIPT = STM32F072C8TX_FLASH.ld

# USER_DEFS user defined macros
USER_DEFS = -D HSI48_VALUE=48000000 -D HSE_VALUE=16000000

# USER_INCLUDES: user defined includes
USER_INCLUDES = -IUSB_DEVICE/Target
USER_INCLUDES += -IUSB_DEVICE/App

# USB_INCLUDES: includes for the usb library
USB_INCLUDES = -IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc
USB_INCLUDES += -IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc

# USER_CFLAGS: user C flags (enable warnings, enable debug info)
USER_CFLAGS = -Os -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F072xB -static --specs=nano.specs -mthumb -mfloat-abi=soft -Wl,--start-group -lc -lm -Wl,--end-group -Wl,--gc-sections  -Wall -fstack-usage -fno-exceptions -ffunction-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fdata-sections

ifneq ($(EXTERNAL_OSCILLATOR), 1)
USER_CFLAGS += -DINTERNAL_OSCILLATOR
endif

# USER_LDFLAGS:  user LD flags
USER_LDFLAGS =

# TARGET_DEVICE: device to compile for
TARGET_DEVICE = STM32F072xB

#######################################
# end of user configuration
#######################################
#
#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
RANLIB = arm-none-eabi-ranlib
SIZE = arm-none-eabi-size
OBJCOPY = arm-none-eabi-objcopy
MKDIR = mkdir -p
#######################################

# core and CPU type for Cortex M0
# ARM core type (CORE_M0, CORE_M3)
CORE = CORE_M0
# ARM CPU type (cortex-m0, cortex-m3)
CPU = cortex-m0

# where to build STM32Cube
CUBELIB_BUILD_DIR = $(BUILD_DIR)/STM32Cube

# various paths within the STmicro library
CMSIS_PATH = Drivers/CMSIS
CMSIS_DEVICE_PATH = $(CMSIS_PATH)/Device/ST/STM32F0xx
DRIVER_PATH = Drivers/STM32F0xx_HAL_Driver

# includes for gcc
INCLUDES = -I$(CMSIS_PATH)/Include
INCLUDES += -I$(CMSIS_DEVICE_PATH)/Include
INCLUDES += -I$(DRIVER_PATH)/Inc
INCLUDES += -ICore/Inc
INCLUDES += $(USB_INCLUDES)
INCLUDES += $(USER_INCLUDES)

# macros for gcc
DEFS = -D$(CORE) $(USER_DEFS) -D$(TARGET_DEVICE)

# compile gcc flags
CFLAGS += $(DEFS) $(INCLUDES)
CFLAGS += -mcpu=$(CPU) -mthumb
CFLAGS += $(USER_CFLAGS)
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
CFLAGS += -DGIT_REMOTE=\"$(GIT_REMOTE)\"

# default action: build the user application
all: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

flash: all
	sudo dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the st micro peripherial library
# (drivers and CMSIS)
#######################################

CUBELIB = $(CUBELIB_BUILD_DIR)/libstm32cube.a

# List of stm32 driver objects
CUBELIB_DRIVER_OBJS = $(addprefix $(CUBELIB_BUILD_DIR)/, $(patsubst %.c, %.o, $(notdir $(wildcard $(DRIVER_PATH)/Src/*.c))))

# shortcut for building core library (make cubelib)
cubelib: $(CUBELIB)

$(CUBELIB): $(CUBELIB_DRIVER_OBJS)
	$(AR) rv $@ $(CUBELIB_DRIVER_OBJS)
	$(RANLIB) $@

$(CUBELIB_BUILD_DIR)/%.o: $(DRIVER_PATH)/Src/%.c | $(CUBELIB_BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $^

$(CUBELIB_BUILD_DIR):
	$(MKDIR) $@

#######################################
# build the USB library
#######################################
USB_MIDDLEWARE_PATH = ./Middlewares/ST/STM32_USB_Device_Library/
USB_BUILD_DIR = $(BUILD_DIR)/usb
USB_SOURCES += usbd_ctlreq.c usbd_ioreq.c usbd_core.c usbd_cdc.c
# list of usb library objects
USB_OBJECTS += $(addprefix $(USB_BUILD_DIR)/,$(notdir $(USB_SOURCES:.c=.o)))

usb: $(USB_OBJECTS)

$(USB_BUILD_DIR)/%.o: $(USB_MIDDLEWARE_PATH)/Core/Src/%.c | $(USB_BUILD_DIR)
	$(CC) -Os $(CFLAGS) -c -o $@ $^

$(USB_BUILD_DIR)/%.o: $(USB_MIDDLEWARE_PATH)/Class/CDC/Src/%.c | $(USB_BUILD_DIR)
	$(CC) -Os $(CFLAGS) -c -o $@ $^

$(USB_BUILD_DIR):
	@echo $(USB_BUILD_DIR)
	$(MKDIR) $@

#######################################
# build the user application
#######################################

# list of user program objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(USB_SOURCES_APP:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(USB_SOURCES_TARGET:.c=.o)))
# add an object for the startup code
OBJECTS += $(BUILD_DIR)/startup_stm32f072c8tx.o

# use the periphlib core library, plus generic ones (libc, libm, libnosys)
LIBS = -lstm32cube -lc -lm -lnosys
LDFLAGS = -T $(LD_SCRIPT) -L $(CUBELIB_BUILD_DIR) -static $(LIBS) $(USER_LDFLAGS)

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $(BUILD_DIR)/$(TARGET).elf $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/$(TARGET).elf $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(USB_OBJECTS) $(CUBELIB)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(USB_OBJECTS) \
		$(LDFLAGS) -Xlinker \
		-Map=$(BUILD_DIR)/$(TARGET).map
	$(SIZE) $@

$(BUILD_DIR)/%.o: Core/Src/%.c | $(BUILD_DIR)
	mkdir -p build
	$(CC) $(CFLAGS) -Os -c -o $@ $^

$(BUILD_DIR)/%.o: USB_DEVICE/App/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Os -c -o $@ $^

$(BUILD_DIR)/%.o: USB_DEVICE/Target/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Os -c -o $@ $^

$(BUILD_DIR)/%.o: Core/Startup/%.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR):
	$(MKDIR) $@

# delete all user application files, keep the libraries
clean:
		-rm $(BUILD_DIR)/*.o
		-rm $(BUILD_DIR)/*.elf
		-rm $(BUILD_DIR)/*.hex
		-rm $(BUILD_DIR)/*.map
		-rm $(BUILD_DIR)/*.bin

.PHONY: clean all cubelib
