#ifndef BACCABLE_STATE_DISPLAY_H
#define BACCABLE_STATE_DISPLAY_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    CAN_TxHeaderTypeDef shift_msg_header;
    uint8_t shift_msg_data[8];
    float scaled_volume;
    uint8_t scaled_color_set;
    uint8_t leds_strip_is_on;
#endif
#if defined(BACCABLE_BH)
    uint32_t last_sent_telematic_display_info_msg_time;
    uint8_t telematic_display_info_field_total_frame_number;
    uint8_t telematic_display_info_field_frame_number;
    uint8_t telematic_display_info_field_info_code;
    uint8_t params_string_char_index;
    CAN_TxHeaderTypeDef telematic_display_info_msg_header;
    uint8_t telematic_display_info_msg_data[8];
    uint8_t request_to_send_one_frame;
    uint8_t chime_msg_data[8];
    CAN_TxHeaderTypeDef chime_msg_header;
    uint8_t request_to_play_chime;
    #if defined(DISABLE_ODOMETER_BLINK)
    uint8_t disable_odometer_blink;
    #endif
#endif
#if defined(BACCABLE_BH)
    #if defined(DISABLE_ODOMETER_BLINK)
    #else
    uint8_t disable_odometer_blink;
    #endif
#endif
} DisplayState;
extern DisplayState display_state;
#endif
