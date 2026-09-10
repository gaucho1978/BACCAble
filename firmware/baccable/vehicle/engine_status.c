#include "vehicle/engine_status.h"

/* Track engine operation and update features that depend on startup or shutdown. */
void vehicle_handle_engine_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 2)
        return;

// message to dashboard containing rpm speed and not only
#if (defined(BACCABLE_C1) || defined(BACCABLE_C2))
    if (rx_header->DLC >= 2) {
        telemetry_state.current_rpm_speed =
            (frame_data[0] * 256 + (frame_data[1] & ~0x3)) / 4; // extract rpm speed
    }
#endif
#if defined(BACCABLE_C2)
    if (telemetry_state.current_rpm_speed < 400) {
        if (chassis_state.stability_inverted != 0)
            chassis_state.stability_inverted = 0;
        if (chassis_state.dyno_mode_enabled != 0)
            chassis_state.dyno_mode_enabled = 0;
        if (chassis_state.front_brake_forced != 0)
            chassis_state.front_brake_forced = 255;
    }
#endif
#if defined(BACCABLE_C1)
    if (telemetry_state.current_rpm_speed < 400) {
        comfort_state.start_and_stop_enabled = 1;            // if motor off, re-enable start&stop logic
        comfort_state.request_to_disable_start_and_stop = 0; // re set request trigger

        if (chassis_state.awd_sequence > 0) {
            // disable 4dw function
            chassis_state.awd_sequence = 0; // disable 4dw function
        }

        if (dashboard_state.baccable_dashboard_menu_visible == 1) {
            if (dashboard_state.shutdown_dashboard_menu_request_time == 0)
                dashboard_state.shutdown_dashboard_menu_request_time =
                    currentTime; // save time. we will shut it off after one minute
        }

        telemetry_state.engine_on_since_more_than5seconds = 0; // engine is off
    } else {
        dashboard_state.shutdown_dashboard_menu_request_time = 0;
        if (dashboard_state.baccabledashboard_menu_was_visible == 1) {
            dashboard_state.baccable_dashboard_menu_visible = 1;    // show menu
            dashboard_state.baccabledashboard_menu_was_visible = 0; // avoid to return here
        }

        if (telemetry_state.engine_on_since_more_than5seconds < 500)
            telemetry_state
                .engine_on_since_more_than5seconds++; // engine is rotating. increase it (this message arrives
                                                      // each 10msec, so if >500, 5 seconds has elapsed)
    }
#endif

    // engine speed fail is on byte1 bit 1.
    // engine StopStart Status is on byte 1 bit 0 and byte 2 bit 7
    // engine Status is on byte 2 bit 6 and bit 5.
    // gas pedal position is on byte 2 from bit 4 to 0 and byte 3 from bit 7 to 5.
    // gas pedal position fail is on byte3 bit 4.
    // reverse gear is on byte3, bit 3 and 2 (value 1=inserted). on C2
    // alternator fail is on byte 3, bit1.
    // stopStart status is on byte3 bit 0 and byte4 bit7.
    // CC brake intervention request is on byte 4, bit5
    // bank deactivation status is on byte5, bit 7 and 6
    // CC brake intervention is on byte 5 from bit 5 to 0 and byte 6 from bit 7 to 4.
}
