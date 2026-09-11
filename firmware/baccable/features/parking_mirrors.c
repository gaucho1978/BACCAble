/* Parking behavior adapted from gaucho1978 and netzmark BACCAble. */
#include "app/powertrain.h"
#include "features/parking_mirrors.h"
#include "state/parking.h"
#if defined(BACCABLE_BH)
static uint32_t exit_at, neutral_at, target_at;
static bool exit_pending, neutral_pending;

/* Lower the selected mirror for reversing and restore it after the maneuver. */
void parking_mirrors_process(void) {
    if (!parking_state.gear_known || !parking_state.rpm_known ||
        currentTime - parking_state.gear_seen > 1000 || currentTime - parking_state.rpm_seen > 1000)
        return;
    bool lowered =
        mirrors_state.left_park_mirror_position_required || mirrors_state.right_park_mirror_position_required;
    bool reverse = settings_state.park_mirror && telemetry_state.current_gear == 0x0e &&
                   telemetry_state.current_rpm_speed > 400;
    if (reverse) {
        exit_pending = neutral_pending = false;
        uint8_t *side = mirrors_state.turn_indicator == 2 ? &mirrors_state.left_park_mirror_position_required
                        : mirrors_state.turn_indicator == 1
                            ? &mirrors_state.right_park_mirror_position_required
                            : NULL;
        if (side && !*side) {
            if (!lowered && !mirrors_state.restore_operative_mirrors_position)
                mirrors_state.store_operative_mirror_position = 1;
            mirrors_state.restore_operative_mirrors_position = 0;
            *side = 1;
            target_at = currentTime;
            lowered = true;
        }
    } else if (lowered) {
        if (!exit_pending) {
            exit_pending = true;
            exit_at = currentTime;
        }
        bool immediate = !settings_state.park_mirror || telemetry_state.current_gear == 0x0d ||
                         telemetry_state.current_rpm_speed <= 400;
        bool neutral = telemetry_state.current_gear == 0;
        if (neutral && !neutral_pending) {
            neutral_pending = true;
            neutral_at = currentTime;
        }
        if (!neutral)
            neutral_pending = false;
        /* Ignore a brief neutral transition while passing between reverse and drive. */
        if (immediate || (neutral_pending && currentTime - neutral_at >= 500) ||
            currentTime - exit_at >= 10000) {
            mirrors_state.left_park_mirror_position_required = 0;
            mirrors_state.right_park_mirror_position_required = 0;
            mirrors_state.restore_operative_mirrors_position = 1;
            mirrors_state.restore_operative_mirrors_position_request_time = currentTime;
            target_at = currentTime;
            lowered = false;
            exit_pending = neutral_pending = false;
        }
    }
    if (mirrors_state.restore_operative_mirrors_position &&
        currentTime - mirrors_state.restore_operative_mirrors_position_request_time > 17500)
        mirrors_state.restore_operative_mirrors_position = 0;
    if ((!lowered && !mirrors_state.restore_operative_mirrors_position) ||
        mirrors_state.store_operative_mirror_position || currentTime - target_at < 2500 ||
        currentTime - mirrors_state.last_park_mirror_msg_time < 900)
        return;
    uint8_t *data = mirrors_state.park_mirror_msg_data;
    data[0] = mirrors_state.left_park_mirror_position_required
                  ? mirrors_state.left_park_mirror_horizontal_pos
                  : mirrors_state.left_mirror_horizontal_operative_pos;
    data[1] = mirrors_state.left_park_mirror_position_required
                  ? mirrors_state.left_park_mirror_vertical_pos
                  : mirrors_state.left_mirror_vertical_operative_pos;
    data[2] = mirrors_state.right_park_mirror_position_required
                  ? mirrors_state.right_park_mirror_horizontal_pos
                  : mirrors_state.right_mirror_horizontal_operative_pos;
    data[3] = mirrors_state.right_park_mirror_position_required
                  ? mirrors_state.right_park_mirror_vertical_pos
                  : mirrors_state.right_mirror_vertical_operative_pos;
    if (can_tx(&mirrors_state.park_mirror_msg_header, data) == HAL_OK)
        mirrors_state.last_park_mirror_msg_time = currentTime;
}
#endif
