#ifndef BACCABLE_STATE_PARKING_H
#define BACCABLE_STATE_PARKING_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
    bool sensor_mute, reverse_audio;
    bool gear_known, brake_known, rpm_known;
    uint32_t rpm_seen;
    bool reverse, beeping, pdc_disabled, pdc_known, pdc_owned;
    uint8_t pressure, pulse;
    uint32_t pulse_at, pdc_seen, gear_seen, brake_seen, alarm_seen;
    uint8_t radio[6], audio_owned;
    bool radio_known, audio_known, audio_muted, audio_waiting;
    uint32_t radio_seen, audio_seen, audio_exit_at;
} ParkingState;
extern ParkingState parking_state;
#endif
