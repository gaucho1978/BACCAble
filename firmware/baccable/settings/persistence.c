#include "features/periodic.h"
#include "app/powertrain.h"
#include "storage/flash_records.h"
#include "features/menu.h"
#include "features/usb_modes.h"
#if defined(BACCABLE_C1)
static uint16_t saved_settings[SETUP_FLASH_PARAM_BUFFER_SIZE];
static uint16_t best_times[2];
static uint8_t settings_loaded, times_loaded;

/* Load saved feature preferences once for subsequent lookups. */
static void load_settings(void) {
    if (settings_loaded)
        return;
    memset(saved_settings, 0xff, sizeof(saved_settings));
    flash_record_load(SETTINGS_RECORD, 0x101, saved_settings, sizeof(saved_settings));
    settings_loaded = 1;
}

/* Load saved performance records once for subsequent lookups. */
static void load_times(void) {
    if (times_loaded)
        return;
    memset(best_times, 0xff, sizeof(best_times));
    flash_record_load(STATISTICS_RECORD, 0x102, best_times, sizeof(best_times));
    times_loaded = 1;
}

/* Return a saved feature preference or its unavailable marker. */
uint16_t settings_read(uint8_t id) {
    if (!id || id > SETUP_FLASH_PARAM_BUFFER_SIZE)
        return 0;
    load_settings();
    return setup_read_flash_value(id, saved_settings[id - 1]);
}

/* Store the current feature preferences for the next startup. */
uint8_t settings_save(void) {
    uint16_t values[SETUP_FLASH_PARAM_BUFFER_SIZE] = {0};
    if (setup_flash_slots_count() > SETUP_FLASH_PARAM_BUFFER_SIZE)
        return 254;
    setup_fill_flash_params(values);
    if (!flash_record_save(SETTINGS_RECORD, 0x101, values, sizeof(values)))
        return 255;
    memcpy(saved_settings, values, sizeof(values));
    settings_loaded = 1;
    board_sync_restart();
    usb_modes_apply();
    return 0;
}

/* Return the best saved acceleration time for the requested interval. */
float statistics_read_best(uint8_t id) {
    if (id < 1 || id > 2)
        return 65.535f;
    load_times();
    return best_times[id - 1] / 1000.0f;
}

/* Clear saved acceleration records and report whether the save succeeded. */
uint8_t statistics_reset(void) {
    uint16_t empty[2] = {UINT16_MAX, UINT16_MAX};
    if (!flash_record_save(STATISTICS_RECORD, 0x102, empty, sizeof(empty)))
        return 255;
    memcpy(best_times, empty, sizeof(empty));
    times_loaded = 1;
    return 0;
}

/* Save improved acceleration records without replacing a better result. */
uint8_t statistics_save_best(void) {
    load_times();
    uint16_t next[2] = {best_times[0], best_times[1]};
    const float seconds[2] = {statistics_state.chronometer_elapsed_time_0_100_km_h,
                              statistics_state.chronometer_elapsed_time_100_200_km_h};
    const uint8_t running[2] = {statistics_state.statistics_0_100_started,
                                statistics_state.statistics_100_200_started};
    const float limits[2] = {20.0f, 40.0f};
    for (unsigned i = 0; i < 2; ++i) {
        if (!running[i] && isfinite(seconds[i]) && seconds[i] > 0 && seconds[i] <= limits[i]) {
            uint16_t milliseconds = (uint16_t)(seconds[i] * 1000.0f + 0.5f);
            if (milliseconds < next[i])
                next[i] = milliseconds;
        }
    }
    if (!memcmp(next, best_times, sizeof(next)))
        return 253;
    if (!flash_record_save(STATISTICS_RECORD, 0x102, next, sizeof(next)))
        return 255;
    memcpy(best_times, next, sizeof(next));
    return 0;
}
#endif
