#ifndef BACCABLE_VEHICLE_STEERING_CONTROLS_H
#define BACCABLE_VEHICLE_STEERING_CONTROLS_H

#include "app/application_state.h"
#include "platform/system.h"
#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
#endif

void vehicle_handle_steering_controls(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif /* INC_PROCESSINGMESSAGE0X00000226_H_ */
