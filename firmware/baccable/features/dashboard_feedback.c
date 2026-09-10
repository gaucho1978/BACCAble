#include "app/powertrain.h"
#include "features/periodic.h"
#if defined(BACCABLE_C1)

/* Provide requested dashboard flashes, warnings and audible feedback. */
void dashboard_feedback_process(void) {
    if (dashboard_state.execute_dashboard_blinks >
        0) { // if we shall execute a blink to give a feedback to the user
        if (currentTime - dashboard_state.last_sent_dashboard_blink_msg_time >
            500) { // enter here once each halfsecond
            dashboard_state.last_sent_dashboard_blink_msg_time = currentTime;
            // change the message
            if (dashboard_state.execute_dashboard_blinks % 2 ==
                0) { // select one time reduced brightness and one time high brightness, so that in any
                     // condition the change is visible on the dashboard
                dashboard_state.dashboard_blink_msg_data[4] = 0x00; // max bright
            } else {
                dashboard_state.dashboard_blink_msg_data[4] = 0xF0; // reduced bright
            }
            dashboard_state.execute_dashboard_blinks =
                dashboard_state.execute_dashboard_blinks - 1; // decrease blinks counter
            status_led_activity();
            // send the message
            can_tx(&dashboard_state.dashboard_blink_msg_header, dashboard_state.dashboard_blink_msg_data);
        }
    }
}
#endif
