#include "state/display.h"
DisplayState display_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .shift_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x2ED, .DLC = 8},
    .leds_strip_is_on = 0,
#endif
#if defined(BACCABLE_BH)
    .last_sent_telematic_display_info_msg_time = 0,
    .telematic_display_info_field_total_frame_number = (DASHBOARD_MESSAGE_MAX_LENGTH / 3) - 1,
    .telematic_display_info_field_frame_number = 0,
    .telematic_display_info_field_info_code = DISPLAY_INFO_CODE,
    .params_string_char_index = 0,
    .telematic_display_info_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x090, .DLC = 8},
    .request_to_send_one_frame = 0,
    .chime_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x5AC, .DLC = 8},
    .request_to_play_chime = 0,
    #if defined(DISABLE_ODOMETER_BLINK)
    .disable_odometer_blink = 1,
    #endif
#endif
#if defined(BACCABLE_BH)
    #if defined(DISABLE_ODOMETER_BLINK)
    #else
    .disable_odometer_blink = 0,
    #endif
#endif
};
