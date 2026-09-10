#ifndef BACCABLE_VEHICLE_DIAGNOSTIC_FRAMES_H
#define BACCABLE_VEHICLE_DIAGNOSTIC_FRAMES_H

#include "app/application_state.h"
#include "app/powertrain.h"

void vehicle_dispatch_diagnostic(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
