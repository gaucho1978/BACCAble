#ifndef BACCABLE_STATE_DASHBOARD_H
#define BACCABLE_STATE_DASHBOARD_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    uint8_t print_stop_the_car;
    uint8_t print_enable_dyno;
    uint32_t shutdown_dashboard_menu_request_time;
    uint8_t checkbox_symbols[2];
    CAN_TxHeaderTypeDef dashboard_blink_msg_header;
    uint8_t dashboard_blink_msg_data[8];
    uint32_t last_sent_dashboard_blink_msg_time;
    uint8_t execute_dashboard_blinks;
    uint8_t main_dashboard_page_index;
    uint8_t dashboard_menu_indent_level;
    uint8_t dashboard_main_menu_array_len;
    uint8_t dashboard_main_menu_array[20][DASHBOARD_MESSAGE_MAX_LENGTH];
    uint8_t baccable_dashboard_menu_visible;
    uint8_t baccabledashboard_menu_was_visible;
    uint8_t dashboard_page_index;
#endif

    uint8_t dashboard_page_string_array[DASHBOARD_MESSAGE_MAX_LENGTH];

    uint8_t commands_menu_enabled;
} DashboardState;
extern DashboardState dashboard_state;
#endif
