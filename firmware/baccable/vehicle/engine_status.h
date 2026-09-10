#ifndef BACCABLE_VEHICLE_ENGINE_STATUS_H
#define BACCABLE_VEHICLE_ENGINE_STATUS_H

#include "app/application_state.h"

void vehicle_handle_engine_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
