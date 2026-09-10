#ifndef BACCABLE_VEHICLE_DRIVE_MODE_H
#define BACCABLE_VEHICLE_DRIVE_MODE_H

#include "app/application_state.h"
#include "platform/system.h"

void vehicle_handle_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
