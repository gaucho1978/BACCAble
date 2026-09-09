#ifndef BACCABLE_DIAGNOSTICS_PARAMETER_REQUEST_H
#define BACCABLE_DIAGNOSTICS_PARAMETER_REQUEST_H
#include "stm32f0xx_hal.h"
void parameter_request_begin(void);
void parameter_request_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
#endif
