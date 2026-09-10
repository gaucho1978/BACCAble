#include "state/statistics.h"
StatisticsState statistics_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .chronometer_elapsed_time_0_100_km_h = 60,
    .chronometer_elapsed_time_100_200_km_h = 60,
    .statistics_0_100_started = 0,
    .statistics_100_200_started = 0,
#endif

    .previous_speed_km_h = 0,

    .statistics_0_100_start_time = 0,

    .statistics_100_200_start_time = 0,
};
