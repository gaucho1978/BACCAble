#ifndef BACCABLE_FAULT_READER_H
#define BACCABLE_FAULT_READER_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "app/build_config.h"
#include "stm32f0xx_hal.h"
void fault_reader_start(uint8_t ecu);
void fault_reader_cancel(void);
void fault_reader_process(void);
void fault_reader_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
bool fault_reader_busy(void);
unsigned fault_reader_count(void);
void fault_reader_text(unsigned index, char *text, size_t capacity);
#endif
