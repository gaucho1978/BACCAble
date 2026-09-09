#include "state/telemetry.h"
TelemetryState telemetry_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .engine_on_since_more_than5seconds = 0,
    .battery_state_of_charge = 0,
    .diesel_engine_regeneration_mode = 0,
    .regeneration_in_progress = 0,
    .loops_from_regeneration_ended = 0,
#endif

    .current_speed_km_h = 0,

    .current_rpm_speed = 0,

    .current_gear = 0,
};
