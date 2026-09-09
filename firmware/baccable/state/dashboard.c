#include "state/dashboard.h"
DashboardState dashboard_state =
    {
        .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
        .print_stop_the_car = 0,
        .print_enable_dyno = 0,
        .shutdown_dashboard_menu_request_time = 0,
        .checkbox_symbols = {0x4F, 0xD8},
        .dashboard_blink_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x545, .DLC = 8},
        .dashboard_blink_msg_data = {0x88, 0x20, 0xC3, 0x24, 0x00, 0x14, 0x30, 0x00},
        .last_sent_dashboard_blink_msg_time = 0,
        .execute_dashboard_blinks = 0,
        .main_dashboard_page_index = 0,
        .dashboard_menu_indent_level = 0,
        .dashboard_main_menu_array_len = 15,
        .dashboard_main_menu_array =
            {
                {},
                {'S', 'h', 'o', 'w', ' ', 'P', 'a', 'r', 'a', 'm', 'e', 't', 'e', 'r', 's', ' ', ' ', ' '},
                {'R', 'e', 'a', 'd', ' ', 'F', 'a', 'u', 'l', 't', 's', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                {'C', 'l', 'e', 'a', 'r', ' ', 'F', 'a', 'u', 'l', 't', 's', ' ', ' ', ' ', ' ', ' ', ' '},
                {'I', 'm', 'm', 'o', 'b', 'i', 'l', 'i', 'z', 'e', 'r', ' ', ' ', 'O', 'N', ' ', ' ', ' '},
                {'T', 'o', 'g', 'g', 'l', 'e', ' ', 'D', 'Y', 'N', 'O', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                {'T', 'o', 'g', 'g', 'l', 'e', ' ', 'E', 'S', 'C', '/', 'T', 'C', ' ', ' ', ' ', ' ', ' '},
                {'F', 'r', 'o', 'n', 't', ' ', 'B', 'r', 'a', 'k', 'e', ' ', 'N', 'o', 'r', 'm', 'a', 'l'},
                {'4', 'W', 'D', ' ', ' ', 'E', 'n', 'a', 'b', 'l', 'e', 'd', ' ', ' ', ' ', ' ', ' ', ' '},
                {'M', 'a', 'i', 'n', ' ', 'S', 'e', 't', 'u', 'p', ' ', 'M', 'e', 'n', 'u', ' ', ' ', ' '},
                {'P', 'a', 'r', 'a', 'm', 's', ' ', 'S', 'e', 't', 'u', 'p', ' ', 'M', 'e', 'n', 'u', ' '},
                {'E', 'n', 'a', 'b', 'l', 'e', ' ', 'H', 'A', 'S', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
                {'T', 'o', 'g', 'g', 'l', 'e', ' ', 'Q', 'V', ' ', 'V', 'a', 'l', 'v', 'e', ' ', ' ', ' '},
                {'S', 'a', 'v', 'e', ' ', 'L', 'o', 'g', ' ', 't', 'o', ' ', 'F', 'i', 'l', 'e', ' ', ' '},
                {'R', 'e', 's', 'e', 't', ' ', 'S', 't', 'a', 't', 'i', 's', 't', 'i', 'c', 's', ' ', ' '},

            },
        .baccable_dashboard_menu_visible = 0,
        .baccabledashboard_menu_was_visible = 0,
        .dashboard_page_index = 0,
#endif

        .dashboard_page_string_array =
            {
                ' ',
            },

        .commands_menu_enabled = 1,
};
