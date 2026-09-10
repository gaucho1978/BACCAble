#include "vehicle/shift_indicator.h"

/* ID 0x2ED: water temperature at byte 0, shift urgency at byte 6 bits 0..1. */
/* Apply the user's shift-indicator preference to dashboard guidance. */
void vehicle_handle_shift_indicator(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 8)
        return;
#if defined(BACCABLE_C1)
    uint32_t rpm = telemetry_state.current_rpm_speed;
    uint32_t threshold = settings_state.shift_threshold;
    if (!settings_state.shift_indicator_enabled || rpm < threshold)
        return;

    memcpy(display_state.shift_msg_data, frame_data, 8);
    if (settings_state.ipc_my23_is_installed)
        display_state.shift_msg_data[1] = (frame_data[1] & ~0x60) | 0x40;
    uint8_t urgency = rpm >= threshold + 1000 ? 3 : rpm >= threshold + 500 ? 2 : 1;
    display_state.shift_msg_data[6] = (frame_data[6] & ~0x03) | urgency;
    if (can_tx(&display_state.shift_msg_header, display_state.shift_msg_data) == HAL_OK)
        status_led_activity();
#endif
}
