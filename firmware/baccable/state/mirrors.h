#ifndef BACCABLE_STATE_MIRRORS_H
#define BACCABLE_STATE_MIRRORS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;

    uint8_t left_mirror_horizontal_operative_pos;

    uint8_t left_mirror_vertical_operative_pos;

    uint8_t right_mirror_horizontal_operative_pos;

    uint8_t right_mirror_vertical_operative_pos;

    uint8_t store_operative_mirror_position;

    uint8_t left_park_mirror_vertical_pos;

    uint8_t left_park_mirror_horizontal_pos;

    uint8_t right_park_mirror_vertical_pos;

    uint8_t right_park_mirror_horizontal_pos;

    uint8_t store_current_park_mirror_position;

    uint8_t park_mirrors_steady;

    uint8_t turn_indicator;

    uint8_t park_mirror_msg_data[8];

    CAN_TxHeaderTypeDef park_mirror_msg_header;

    uint32_t last_park_mirror_msg_time;

    uint32_t restore_operative_mirrors_position_request_time;

    uint8_t restore_operative_mirrors_position;

    uint8_t left_park_mirror_position_required;

    uint8_t right_park_mirror_position_required;

    uint8_t park_mirror_operative_position_not_stored;
} MirrorsState;
extern MirrorsState mirrors_state;
#endif
