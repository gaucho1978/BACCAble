#ifndef BACCABLE_STATE_STATISTICS_H
#define BACCABLE_STATE_STATISTICS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    float chronometer_elapsed_time_0_100_km_h;
    float chronometer_elapsed_time_100_200_km_h;
    uint8_t statistics_0_100_started;
    uint8_t statistics_100_200_started;
#endif

    float previous_speed_km_h;

    uint32_t statistics_0_100_start_time;

    uint32_t statistics_100_200_start_time;
} StatisticsState;
extern StatisticsState statistics_state;
#endif
