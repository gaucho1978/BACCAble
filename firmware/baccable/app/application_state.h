#ifndef BACCABLE_APP_APPLICATION_STATE_H
#define BACCABLE_APP_APPLICATION_STATE_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
#include <string.h>
#include <math.h>
#include "state/pedal.h"
#include "state/settings.h"
#include "state/statistics.h"
#include "state/dashboard.h"
#include "state/comfort.h"
#include "state/security.h"
#include "state/telemetry.h"
#include "state/diagnostics.h"
#include "state/chassis.h"
#include "state/display.h"
#include "state/runtime.h"
#include "state/mirrors.h"
#include "platform/status_led.h"
#include "transport/can_bus.h"
#include "transport/board_uart.h"
#include "protocol/slcan.h"
#include "diagnostics/parameter_catalog.h"
#include "platform/led_strip.h"
#include "platform/power.h"
#if defined(ACT_AS_CANABLE) || defined(DEBUG_MODE) || defined(ENABLE_USB_MASS_STORAGE) ||                    \
    defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    #include "usb_device.h"
    #ifdef ENABLE_USB_MASS_STORAGE
        #include "usbd_storage_if.h"
        #include "third_party/fatfs/ff.h"
    #else
        #include "usbd_cdc_if.h"
    #endif
#endif
extern const char *FW_VERSION;
extern const uint8_t led_light_on_bit;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
#endif
