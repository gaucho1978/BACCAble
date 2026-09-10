#include "state/mirrors.h"
MirrorsState mirrors_state = {
    .reserved = 0,

    .left_mirror_horizontal_operative_pos = 0,

    .left_mirror_vertical_operative_pos = 0,

    .right_mirror_horizontal_operative_pos = 0,

    .right_mirror_vertical_operative_pos = 0,

    .store_operative_mirror_position = 0,

    .left_park_mirror_vertical_pos = 0,

    .left_park_mirror_horizontal_pos = 0,

    .right_park_mirror_vertical_pos = 0,

    .right_park_mirror_horizontal_pos = 0,

    .store_current_park_mirror_position = 0,

    .park_mirrors_steady = 1,

    .turn_indicator = 0,

    .park_mirror_msg_data = {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00},

    .park_mirror_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x5A8, .DLC = 8},

    .last_park_mirror_msg_time = 0,

    .restore_operative_mirrors_position_request_time = 0,

    .restore_operative_mirrors_position = 0,

    .left_park_mirror_position_required = 0,

    .right_park_mirror_position_required = 0,

    .park_mirror_operative_position_not_stored = 1,
};
