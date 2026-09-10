#ifndef BACCABLE_PLATFORM_SYSTEM_H
#define BACCABLE_PLATFORM_SYSTEM_H
#include "app/application_state.h"

uint8_t frame_checksum(uint8_t *data, uint8_t arraySize);

void SystemClock_Config(void);
void Error_Handler(uint16_t halfPeriod);
void filesystem_save_log(void);

void filesystem_init(void);
#endif /* BACCABLE_PLATFORM_SYSTEM_H */
