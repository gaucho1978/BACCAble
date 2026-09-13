CC := clang
FIRMWARE := ../firmware/baccable
CPPFLAGS := -I$(FIRMWARE)
CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
HEADERS := test_menu_contract.c test_report.h test_setup_ui.c test_unified_ui.c $(shell find stubs $(FIRMWARE)/app $(FIRMWARE)/protocol $(FIRMWARE)/diagnostics $(FIRMWARE)/storage $(FIRMWARE)/features $(FIRMWARE)/transport $(FIRMWARE)/platform $(FIRMWARE)/USB_DEVICE/App $(FIRMWARE)/third_party/printf -name '*.h')
SOURCES := test_core.c $(FIRMWARE)/protocol/slcan_codec.c $(FIRMWARE)/diagnostics/uds_decode.c \
 $(FIRMWARE)/storage/record_store.c

test: build/test_core build/test_transport build/test_catalog build/test_menu build/test_menu_large build/test_uart_menu build/test_input_repeat
	./build/test_core
	./build/test_transport
	./build/test_catalog
	./build/test_catalog_large
	./build/test_menu
	./build/test_menu_large
	./build/test_menu_debug
	./build/test_uart_menu
	./build/test_menu_diagnostics
	./build/test_input_repeat
	./build/test_upstream
	./build/test_parking_c2
	./build/test_parking_bh

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

build/test_input_repeat: test_input_repeat.c $(FIRMWARE)/features/menu_input.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) test_input_repeat.c $(FIRMWARE)/features/menu_input.c -o $@

MENU_SOURCES := test_menu.c $(addprefix $(FIRMWARE)/, features/menu.c features/menu_model.c \
 features/menu_input.c features/ui_entry.c features/menu_diagnostics.c features/display_stream.c features/ibs_override.c features/board_sync.c features/dashboard.c features/dashboard_format.c \
 diagnostics/parameter_cache.c diagnostics/parameter_request.c diagnostics/parameter_catalog.c \
 protocol/checksum.c vehicle/steering_controls.c diagnostics/uds_decode.c diagnostics/fault_reader.c settings/persistence.c settings/setup_menu.c settings/setup_entries.c \
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

UPSTREAM_SOURCES := test_upstream.c $(addprefix $(FIRMWARE)/, features/ui_entry.c diagnostics/fault_reader.c protocol/elm327.c transport/diagnostic_link.c features/ibs_override.c features/usb_modes.c third_party/printf/printf.c)
build/test_upstream: $(UPSTREAM_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) -I$(FIRMWARE)/USB_DEVICE/App $(CFLAGS) -DC1_FLAVOR=1 -DGIT_VERSION=\"test\" $(UPSTREAM_SOURCES) -lm -o $@

test: build/test_upstream

PARKING_SOURCES := test_parking_link.c $(addprefix $(FIRMWARE)/, features/parking.c features/parking_mirrors.c state/parking.c transport/diagnostic_link.c)
build/test_parking_c2: $(PARKING_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC2_FLAVOR=1 -DGIT_VERSION=\"test\" $(PARKING_SOURCES) -lm -o $@
build/test_parking_bh: $(PARKING_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DBH_FLAVOR=1 -DGIT_VERSION=\"test\" $(PARKING_SOURCES) -lm -o $@

test: build/test_parking_c2 build/test_parking_bh

build/test_catalog_large: test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DLARGE_DISPLAY -DGIT_VERSION=\"test\" test_catalog.c $(FIRMWARE)/diagnostics/parameter_catalog.c -lm -o $@

test: build/test_catalog_large

build/test_menu_diagnostics: test_menu_diagnostics.c $(FIRMWARE)/features/menu_diagnostics.c $(HEADERS) Makefile
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -DMENU_DIAGNOSTICS test_menu_diagnostics.c $(FIRMWARE)/features/menu_diagnostics.c -o $@

test: build/test_menu_diagnostics

build/test_menu_debug: $(MENU_SOURCES) $(HEADERS) Makefile
	@mkdir -p build
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) -DC1_FLAVOR=1 -DMENU_DIAGNOSTICS -DGIT_VERSION=\"test\" $(MENU_SOURCES) -lm -o $@

test: build/test_menu_debug
