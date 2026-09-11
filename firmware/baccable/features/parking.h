#ifndef BACCABLE_FEATURES_PARKING_H
#define BACCABLE_FEATURES_PARKING_H
#include "app/build_config.h"
#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx_hal.h"
void parking_set_options(bool sensor_mute, bool reverse_audio);
void parking_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void parking_process(void);
#endif
