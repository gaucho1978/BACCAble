CC := clang
FIRMWARE := ../firmware/baccable
CPPFLAGS := -I$(FIRMWARE)
CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
HEADERS := $(shell find stubs $(FIRMWARE)/app $(FIRMWARE)/protocol $(FIRMWARE)/diagnostics $(FIRMWARE)/storage $(FIRMWARE)/features $(FIRMWARE)/transport $(FIRMWARE)/platform $(FIRMWARE)/USB_DEVICE/App $(FIRMWARE)/third_party/printf -name '*.h')
SOURCES := test_core.c $(FIRMWARE)/protocol/slcan_codec.c $(FIRMWARE)/diagnostics/uds_decode.c \
 $(FIRMWARE)/storage/record_store.c

test: build/test_core build/test_transport build/test_catalog build/test_menu build/test_menu_large build/test_uart_menu
	./build/test_core
	./build/test_transport
	./build/test_catalog
	./build/test_menu
	./build/test_menu_large
	./build/test_uart_menu

build/test_catalog: test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DGIT_VERSION=\"test\" test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c -lm -o $@

TRANSPORT_SOURCES := test_transport.c $(FIRMWARE)/transport/can_bus.c $(FIRMWARE)/USB_DEVICE/App/usbd_cdc_if.c
build/test_transport: $(TRANSPORT_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DGIT_VERSION=\"test\" $(TRANSPORT_SOURCES) -o $@

build/test_core: $(SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -lm -o $@

.PHONY: test

MENU_SOURCES := test_menu.c $(addprefix $(FIRMWARE)/, features/menu.c features/menu_model.c \
 features/menu_input.c features/display_stream.c features/dashboard.c features/dashboard_format.c \
 diagnostics/parameter_cache.c diagnostics/parameter_request.c diagnostics/parameter_catalog.c \
 diagnostics/uds_decode.c settings/persistence.c settings/setup_menu.c settings/setup_entries.c \
 third_party/printf/printf.c)
build/test_menu: $(MENU_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@
build/test_menu_large: $(MENU_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DLARGE_DISPLAY -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@

build/test_uart_menu: test_uart_menu.c $(FIRMWARE)/transport/board_uart.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" test_uart_menu.c $(FIRMWARE)/transport/board_uart.c -o $@
