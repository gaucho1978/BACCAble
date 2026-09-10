
#ifndef BACCABLE_PLATFORM_POWER_H
#define BACCABLE_PLATFORM_POWER_H

#include "app/application_state.h"

#define CHIP_LOW_CONSUME_Pin GPIO_PIN_4
#define CHIP_LOW_CONSUME_Port GPIOA
#define CHIP_LOW_CONSUME CHIP_LOW_CONSUME_Port, CHIP_LOW_CONSUME_Pin

#define CAN_LOW_CONSUME_Pin GPIO_PIN_5
#define CAN_LOW_CONSUME_Port GPIOA
#define CAN_LOW_CONSUME CAN_LOW_CONSUME_Port, CAN_LOW_CONSUME_Pin

#define CHIP_RESET_DURATION 500 // time in milliseconds

void power_init(void);
void power_hold_slaves_in_reset(void);
void power_release_slaves(void);
void power_transceivers_sleep(void);
void power_transceivers_wake(void);
void power_process(void);
void power_sleep(void);
void power_wake(void);

#endif /* BACCABLE_PLATFORM_POWER_H */
