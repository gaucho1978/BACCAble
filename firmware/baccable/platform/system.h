#ifndef BACCABLE_PLATFORM_SYSTEM_H
#define BACCABLE_PLATFORM_SYSTEM_H
#include "app/application_state.h"

void format_number(char *str, float num, uint8_t precision, uint8_t maxLen);

uint8_t frame_checksum(uint8_t *data, uint8_t arraySize);

void SystemClock_Config(void);
void system_irq_disable(void);
void system_irq_enable(void);
void system_hex32(char *out, uint32_t val);
void Error_Handler(uint16_t halfPeriod);
void filesystem_save_log(void);

void filesystem_init(void);
#endif /* BACCABLE_PLATFORM_SYSTEM_H */
