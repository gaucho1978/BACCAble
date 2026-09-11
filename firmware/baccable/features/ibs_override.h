#ifndef BACCABLE_IBS_OVERRIDE_H
#define BACCABLE_IBS_OVERRIDE_H
#include <stdbool.h>
#include "app/build_config.h"
#include "stm32f0xx_hal.h"
void ibs_override_enable(bool enabled);
bool ibs_override_enabled(void);
void ibs_override_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void ibs_override_process(void);
#endif
