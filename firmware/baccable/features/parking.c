/* Adapted from gaucho1978 and netzmark BACCAble parking controls (August–September 2026). */
#include "features/parking.h"
#include "state/parking.h"
#include "state/telemetry.h"
#include "transport/can_bus.h"
#include "platform/system.h"
#include <string.h>

/* Apply the user's parking preferences without forgetting a button release still owed. */
void parking_set_options(bool sensor_mute, bool reverse_audio) {
    parking_state.sensor_mute = sensor_mute;
    parking_state.reverse_audio = reverse_audio;
}

/* Remember actual parking and audio reports before deciding whether automation is appropriate. */
void parking_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *d) {
    if (h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA)
        return;
#if defined(BACCABLE_C2)
    switch (h->StdId) {
    case 0xfc:
        if (h->DLC >= 4) {
            parking_state.reverse = ((d[3] >> 2) & 3) == 1;
            parking_state.gear_seen = currentTime;
            parking_state.gear_known = true;
        }
        break;
    case 0x1f5:
        if (h->DLC >= 5) {
            parking_state.pressure = d[4];
            parking_state.brake_seen = currentTime;
            parking_state.brake_known = true;
        }
        break;
    case 0x3e7:
        if (h->DLC >= 6) {
            parking_state.beeping = d[0] != 0;
            parking_state.alarm_seen = currentTime;
        }
        break;
    case 0x54a:
        if (h->DLC >= 4) {
            parking_state.pdc_disabled = ((d[3] >> 6) & 3) == 1;
            parking_state.pdc_known = true;
            parking_state.pdc_seen = currentTime;
        }
        break;
    default:
        break;
    }
#elif defined(BACCABLE_BH)
    if (h->StdId == 0x3e6 && h->DLC >= 6) {
        parking_state.rpm_known = true;
        parking_state.rpm_seen = currentTime;
        telemetry_state.current_rpm_speed =
            ((uint32_t)(d[3] & 7) << 11) | ((uint32_t)d[4] << 3) | (d[5] >> 5);
    }
    if (h->StdId == 0x3e8 && h->DLC >= 8) {
        parking_state.gear_known = true;
        parking_state.gear_seen = currentTime;
    }
    if (h->StdId == 0x358 && h->DLC >= 6 && d[2] != 0xe0) {
        memcpy(parking_state.radio, d, 6);
        parking_state.radio_known = true;
        parking_state.radio_seen = currentTime;
    }
    if (h->StdId == 0x5be && h->DLC >= 1 && (d[0] == 0x10 || d[0] == 0x30)) {
        bool muted = d[0] == 0x30;
        if (parking_state.audio_muted && !muted && parking_state.audio_owned == 1)
            parking_state.audio_owned = 2; /* Respect a manual unmute for this reverse maneuver. */
        parking_state.audio_muted = muted;
        parking_state.audio_known = true;
        parking_state.audio_seen = currentTime;
    }
#endif
}

#if defined(BACCABLE_C2) || defined(BACCABLE_BH)
/* Send a parking-button press or release, retaining the state when the CAN queue is busy. */
static bool parking_send(uint32_t id, uint8_t *data, uint8_t length) {
    CAN_TxHeaderTypeDef h = {.StdId = id, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = length};
    return can_tx(&h, data) == HAL_OK;
}

#endif

/* Mute only during the supported maneuver and restore only changes made by BACCAble. */
void parking_process(void) {
#if defined(BACCABLE_C2)
    uint8_t command[8] = {0};
    if (parking_state.pulse) {
        if (currentTime - parking_state.pulse_at >= 50 && parking_send(0x5b0, command, 8))
            parking_state.pulse = 0;
        return;
    }
    if (!parking_state.gear_known || !parking_state.brake_known || !parking_state.pdc_known ||
        currentTime - parking_state.pdc_seen > 3000 || currentTime - parking_state.gear_seen > 1000 ||
        currentTime - parking_state.brake_seen > 1000)
        return;
    if (parking_state.reverse) {
        parking_state.pdc_owned = false;
        return;
    }
    bool disable = parking_state.sensor_mute && parking_state.pressure > 0x10 && parking_state.beeping &&
                   currentTime - parking_state.alarm_seen < 3000 && !parking_state.pdc_disabled &&
                   !parking_state.pdc_owned;
    bool restore = parking_state.pdc_owned && parking_state.pdc_disabled &&
                   (!parking_state.sensor_mute || parking_state.pressure < 0x10);
    if (disable || restore) {
        command[1] = 0x20;
        if (parking_send(0x5b0, command, 8)) {
            parking_state.pdc_owned = disable;
            parking_state.pulse = 1;
            parking_state.pulse_at = currentTime;
        }
    } else if (parking_state.pdc_owned && parking_state.pressure < 0x10 && !parking_state.pdc_disabled) {
        parking_state.pdc_owned = false;
    }
#elif defined(BACCABLE_BH)
    if (!parking_state.radio_known)
        return;
    uint8_t command[6];
    memcpy(command, parking_state.radio, 6);
    if (parking_state.pulse) {
        if (currentTime - parking_state.pulse_at >= 50 && parking_send(0x358, command, 6))
            parking_state.pulse = 0;
        return;
    }
    if (!parking_state.gear_known || currentTime - parking_state.gear_seen > 1000 ||
        !parking_state.audio_known || currentTime - parking_state.radio_seen > 3000 ||
        currentTime - parking_state.audio_seen > 3000)
        return;
    bool reverse = telemetry_state.current_gear == 0x0e && parking_state.reverse_audio;
    bool toggle = false;
    uint8_t ownership = parking_state.audio_owned;
    if (reverse) {
        parking_state.audio_waiting = false;
        if (!ownership) {
            if (parking_state.audio_muted)
                parking_state.audio_owned = 2;
            else {
                toggle = true;
                ownership = 1;
            }
        }
    } else {
        bool immediate = !parking_state.reverse_audio || telemetry_state.current_gear == 0x0d ||
                         telemetry_state.current_gear == 0;
        if (!parking_state.audio_waiting) {
            parking_state.audio_waiting = true;
            parking_state.audio_exit_at = currentTime;
        }
        if (immediate || currentTime - parking_state.audio_exit_at >= 3000) {
            toggle = ownership == 1 && parking_state.audio_muted;
            ownership = 0;
            if (!toggle)
                parking_state.audio_owned = 0;
        }
    }
    if (toggle) {
        command[2] = 0xe0;
        if (parking_send(0x358, command, 6)) {
            parking_state.audio_owned = ownership;
            parking_state.pulse = 1;
            parking_state.pulse_at = currentTime;
        }
    }
#endif
}
