#include "state/dashboard.h"
DashboardState dashboard_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .shutdown_dashboard_menu_request_time = 0,
    .dashboard_blink_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x545, .DLC = 8},
    .dashboard_blink_msg_data = {0x88, 0x20, 0xC3, 0x24, 0x00, 0x14, 0x30, 0x00},
    .last_sent_dashboard_blink_msg_time = 0,
    .execute_dashboard_blinks = 0,
    .baccable_dashboard_menu_visible = 0,
    .baccabledashboard_menu_was_visible = 0,
    .dashboard_page_index = 0,
#endif

};
