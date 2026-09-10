#ifndef BACCABLE_VEHICLE_START_STOP_STATUS_H
#define BACCABLE_VEHICLE_START_STOP_STATUS_H

#include "app/application_state.h"

void vehicle_handle_start_stop_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif /* BACCABLE_VEHICLE_START_STOP_STATUS_H */
