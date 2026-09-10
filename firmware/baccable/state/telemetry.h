#ifndef BACCABLE_STATE_TELEMETRY_H
#define BACCABLE_STATE_TELEMETRY_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    uint16_t engine_on_since_more_than5seconds;
    uint8_t oil_pressure;
    uint8_t oil_temperature;
    int16_t torque;
    uint8_t battery_state_of_charge;
    uint16_t battery_current;
    uint8_t diesel_engine_regeneration_mode;
    uint8_t regeneration_in_progress;
    uint8_t loops_from_regeneration_ended;
#endif

    float current_speed_km_h;

    uint32_t current_rpm_speed;

    uint8_t current_gear;

    uint8_t drive_mode;
} TelemetryState;
extern TelemetryState telemetry_state;
#endif
