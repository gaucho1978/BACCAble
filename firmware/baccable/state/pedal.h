#ifndef BACCABLE_STATE_PEDAL_H
#define BACCABLE_STATE_PEDAL_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    uint8_t current_schizzaforte_map;
    uint8_t pedal_map_power_adapted;
    uint32_t last_queued_serial_to_schizza_forte_msg_time;
    uint8_t play_motor_jingle;
    uint8_t jingle_array[255];
#endif
} PedalState;
extern PedalState pedal_state;
#endif
