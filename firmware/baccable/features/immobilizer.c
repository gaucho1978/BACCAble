#include "app/powertrain.h"
#include "features/periodic.h"
#if defined(BACCABLE_C1)

/* Apply the configured immobilizer behavior while its vehicle conditions hold. */
void immobilizer_process(void) {
    if (security_state.immobilizer_enabled) {
        // the following it is used only by IMMOBILIZER functionality
        if (security_state.flood_the_bus) { // WHEN THIS IS ACTIVATED, THE THIEF WILL NOT BE ABLE TO CONNECT
                                            // TO RFHUB, AND CAR WILL NOT SWITCH ON.
            if (currentTime - security_state.flood_the_bus_last_time_sent > 10) {
                can_tx(&security_state.panic_alarm_start_msg_header[0],
                       security_state.panic_alarm_start_msg_data[0]); // sends the message on can bus that
                                                                      // resets the connection to RFHUB
                security_state.flood_the_bus_last_time_sent = currentTime;
            }

            if (!security_state
                     .panic_alarm_activated) { // if panic alarm is not activated, we shall activate it after
                                               // 1 second (1 second to avoid stop and start simultaneous)
                if (currentTime - security_state.flood_the_bus_start_time > 1000) {
                    for (uint8_t i = 0; i < 15; i++) {
                        can_tx(&security_state.panic_alarm_start_msg_header[1],
                               security_state.panic_alarm_start_msg_data[1]);
                    }
                    can_tx(&security_state.panic_alarm_start_msg_header[2],
                           security_state.panic_alarm_start_msg_data[2]);
                    security_state.panic_alarm_activated = 1;
                }
            }
            if (currentTime - security_state.flood_the_bus_start_time >
                10000) {                          // if the bus is flooded since 10 seconds, stop flooding it
                security_state.flood_the_bus = 0; // stop flooding
                // stop the panic alarm
                for (uint8_t i = 0; i < 15; i++) {
                    can_tx(&security_state.panic_alarm_start_msg_header[1],
                           security_state.panic_alarm_start_msg_data[1]);
                }
                can_tx(&security_state.panic_alarm_start_msg_header[2],
                       security_state.panic_alarm_start_msg_data[2]);
                security_state.panic_alarm_activated = 0;
            }
        }
    }
}
#endif
