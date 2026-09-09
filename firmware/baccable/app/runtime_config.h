#ifndef BACCABLE_APP_RUNTIME_CONFIG_H
#define BACCABLE_APP_RUNTIME_CONFIG_H
#include "app/build_config.h"
#define currentTime HAL_GetTick()
#define ENGINE_NORMAL 0
#define ENGINE_DPF_REGEN_LO 1
#define ENGINE_DPF_REGEN_HI 2
#define ENGINE_NSC_DE_NOX_REGEN 3
#define ENGINE_NSC_DE_SOX_REGEN 4
#define ENGINE_SCR_HEATUP_STRATEGY 5
#define UART_BUFFER_SIZE DASHBOARD_MESSAGE_MAX_LENGTH + 1
#define UART1_BUFFER_SIZE 9                        // legth for schizzaforte messages
#define TIMING__ALL___SERIAL_IGNORE_WINDOW_MS 2000 // msec
#define TIMING__C2_BH_USB_CONNECT_TO_C1_NOTIFICATION_DELAY_MS                                                \
    TIMING__ALL___SERIAL_IGNORE_WINDOW_MS + 100 // msec
#define TIMING__C1____DELAY_BEFORE_SERIAL_PROCESS_AFTER_OTHER_CHIP_WAKE_MS                                   \
    TIMING__ALL___SERIAL_IGNORE_WINDOW_MS + 100 // msec
#define TIMING__C1____DELAY_BEFORE_OTHER_CHIP_STATUS_REQUEST_MS                                              \
    TIMING__ALL___SERIAL_IGNORE_WINDOW_MS + 1000                  // msec
#define TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS 3500 // msec
#define TIMING__C1____CAN_ACTIVITY_WINDOW_FOR_WAKEUP_MS                                                      \
    TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS - 100 // msec
#define TIMING__C1____DELAY_BEFORE_SERIAL_INSTRUCT_OF_C2BH_AFTER_OTHER_CHIP_WAKE_MS                          \
    TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS + 2                                        // msec
#define TIMING__C1____C2_STATUS_REQUEST_TIMEOUT_MS 1010                                             // msec
#define TIMING__C1____BH_STATUS_REQUEST_TIMEOUT_MS TIMING__C1____C2_STATUS_REQUEST_TIMEOUT_MS + 250 // msec
#define TIMING__C1____SERIAL_TIMEOUT_REPLY_MS 250                                                   // msec
#define TIMING__C2_BH_SERIAL_TIMEOUT_REPLY_MS TIMING__C1____SERIAL_TIMEOUT_REPLY_MS - 50            // msec
#define TIMING__C1____SCHIZZAFORTE_SERIAL_TIMEOUT_REPLY_MS 100                                      // msec

#endif
