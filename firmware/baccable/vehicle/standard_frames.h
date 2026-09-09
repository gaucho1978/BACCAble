#ifndef BACCABLE_VEHICLE_STANDARD_FRAMES_H
#define BACCABLE_VEHICLE_STANDARD_FRAMES_H

#include "app/application_state.h"
#include "vehicle/engine_status.h"
#include "vehicle/body_commands.h"
#include "vehicle/start_stop_status.h"
#include "vehicle/shift_indicator.h"
#include "vehicle/steering_controls.h"
#include "vehicle/drive_mode.h"

#if defined(BACCABLE_C2)
    #include "features/chassis.h"
#endif
#if defined(BACCABLE_BH)
    #include "features/body.h"
#endif

void vehicle_dispatch_standard(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);

#endif
