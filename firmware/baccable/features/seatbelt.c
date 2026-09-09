#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void seatbelt_process(void) {
    if (((diagnostics_state.seatbelt_alarm_disabled == 0xfe) ||
         (diagnostics_state.seatbelt_alarm_disabled == 0x10) ||
         (diagnostics_state.seatbelt_alarm_disabled == 0x20) ||
         (diagnostics_state.seatbelt_alarm_disabled == 0x11) ||
         (diagnostics_state.seatbelt_alarm_disabled == 0x21)) &&
        (currentTime - diagnostics_state.seatbelt_alarm_status_request_time >
         10000)) {                                        // if operations in progress but timeout was reached
        diagnostics_state.seatbelt_alarm_disabled = 0xff; // return to unknown status
    }

    if (((diagnostics_state.seatbelt_alarm_disabled == 1) ||
         (diagnostics_state.seatbelt_alarm_disabled == 0xff)) &&
        (settings_state.seatbelt_alarm_enabled ==
         1)) { // the alarm is disabled (or unknown) and the use wants to enable it
        if (telemetry_state.engine_on_since_more_than5seconds >= 200) {
            // request to enable Seatbelt Alarm
            // send diag session request
            diagnostics_state.uds_parameter_request_msg_header.ExtId = 0x18DA60F1;
            diagnostics_state.uds_parameter_request_msg_data[0] = 0x02;
            diagnostics_state.uds_parameter_request_msg_data[1] = 0x10;
            diagnostics_state.uds_parameter_request_msg_data[2] = 0x03;
            diagnostics_state.uds_parameter_request_msg_header.DLC = 3;
            can_tx(&diagnostics_state.uds_parameter_request_msg_header,
                   diagnostics_state.uds_parameter_request_msg_data); // transmit the diag session request

            diagnostics_state.seatbelt_alarm_disabled =
                0x20; // request to enable SeatBelt alarm in progress(send write param)
            diagnostics_state.seatbelt_alarm_status_request_time = currentTime;
            diagnostics_state.last_sent_uds_parameter_request_time = currentTime;
        }
    }

    if ((diagnostics_state.seatbelt_alarm_disabled == 0 ||
         diagnostics_state.seatbelt_alarm_disabled == 0xff) &&
        settings_state.seatbelt_alarm_enabled ==
            0) { // alarm is enabled (or unknown) and user wants to disable it
        if (telemetry_state.engine_on_since_more_than5seconds >= 200) {
            // request to disable Seatbelt Alarm
            // send diag session request
            diagnostics_state.uds_parameter_request_msg_header.ExtId = 0x18DA60F1;
            diagnostics_state.uds_parameter_request_msg_data[0] = 0x02;
            diagnostics_state.uds_parameter_request_msg_data[1] = 0x10;
            diagnostics_state.uds_parameter_request_msg_data[2] = 0x03;
            diagnostics_state.uds_parameter_request_msg_header.DLC = 3;
            can_tx(&diagnostics_state.uds_parameter_request_msg_header,
                   diagnostics_state.uds_parameter_request_msg_data); // transmit the diag session request

            diagnostics_state.seatbelt_alarm_disabled =
                0x10; // request to disable SeatBelt alarm in progress(send write param)
            diagnostics_state.seatbelt_alarm_status_request_time = currentTime;
            diagnostics_state.last_sent_uds_parameter_request_time = currentTime;
        }
    }
}
#endif
