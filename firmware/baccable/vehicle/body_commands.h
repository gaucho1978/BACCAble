#ifndef BACCABLE_VEHICLE_BODY_COMMANDS_H
#define BACCABLE_VEHICLE_BODY_COMMANDS_H

#include "app/application_state.h"
#include "platform/system.h"

void vehicle_handle_body_commands(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
