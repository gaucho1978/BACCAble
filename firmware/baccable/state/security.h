#ifndef BACCABLE_STATE_SECURITY_H
#define BACCABLE_STATE_SECURITY_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    uint8_t immobilizer_enabled;
    uint8_t panic_alarm_activated;
    CAN_TxHeaderTypeDef panic_alarm_start_msg_header[5];
    uint8_t panic_alarm_start_msg_data[5][8];
    uint8_t flood_the_bus;
    uint32_t flood_the_bus_start_time;
    uint32_t flood_the_bus_last_time_sent;
#endif
} SecurityState;
extern SecurityState security_state;
#endif
