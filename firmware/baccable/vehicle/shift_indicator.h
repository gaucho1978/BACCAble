#ifndef BACCABLE_VEHICLE_SHIFT_INDICATOR_H
#define BACCABLE_VEHICLE_SHIFT_INDICATOR_H

#include "app/application_state.h"

void vehicle_handle_shift_indicator(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
