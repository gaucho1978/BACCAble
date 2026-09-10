#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)

/* Keep the pedal controller aligned with the selected driving preferences. */
void pedal_booster_process(void) {
    if (settings_state.pedal_booster_enabled) { // if enabled, communicate with schizzaForte each 250msec
        if (telemetry_state.current_rpm_speed < 400)
            pedal_state.current_schizzaforte_map =
                '-'; // if engine is stopped. Forget the pedal map setting, so that baccable will be forced to
                     // set it again on schizzaforte
        if (pedal_booster_needs_update() &&
            (telemetry_state.current_rpm_speed >
             400)) { // only if the map command was not correctly received and engine is on
            if ((currentTime - pedal_state.last_queued_serial_to_schizza_forte_msg_time) >
                (4 * TIMING__C1____SCHIZZAFORTE_SERIAL_TIMEOUT_REPLY_MS)) {
                pedal_state.last_queued_serial_to_schizza_forte_msg_time = currentTime; // avoid to Return
                                                                                        // Here

                if (settings_state.pedal_booster_enabled ==
                    1) { // 1=auto //set mode according to DNAR current selection
                    // function_pedal_booster_enabled: 0=disabled, 1=Automatic Map, 2=Bypass, 3=All Weather
                    // Map, 4=Natural Map, 5=Dynamic Map, 6=Race Map
                    switch (telemetry_state.drive_mode) {
                    case 0x00: // Natural
                        pedal_booster_set_map(4);
                        break;
                    case 0x08: // Dynamic
                        pedal_booster_set_map(5);
                        break;
                    case 0x10: // All Weather
                        pedal_booster_set_map(3);
                        break;
                    case 0x30: // Race
                        pedal_booster_set_map(6);
                        break;
                    default:
                        pedal_booster_set_map(2); // bypass
                    }
                } else {

                    pedal_booster_set_map(settings_state.pedal_booster_enabled);
                }
            }
        }
    }

    if (pedal_state.play_motor_jingle) { // if  jingle was requested, check if we have to disable it
        if (telemetry_state.drive_mode != 0 ||
            telemetry_state.current_rpm_speed < 400) { // engine off or drive style not Natural
            pedal_state.play_motor_jingle = 0;
        }
    }

    if (pedal_state.play_motor_jingle) { // if we have to execute jingle

        // once each 400msec play one note with the engine
        if ((currentTime - pedal_state.last_queued_serial_to_schizza_forte_msg_time) >
            (4 * TIMING__C1____SCHIZZAFORTE_SERIAL_TIMEOUT_REPLY_MS)) {
            pedal_state.last_queued_serial_to_schizza_forte_msg_time = currentTime; // avoid to Return Here
            // play it: force motor RPM for one time slot

            /*
             * Pedal protocol: '#' prefix, command, payload, checksum.
             * 0x6d queries the map. 0xb6 sets a map: byte 2 selects
             * Bypass=0x00, All Weather=0x49, Natural=0x92, Dynamic=0xdb,
             * Race=0x24; bytes 3..7 contain the polynomial coefficients.
             * 0xff sets accelerator value in byte 2 (0..255); bytes 3..7 are zero.
             * For both setters byte 8 is the checksum.
             * Replies identify B/A/N/D/R as 0x10/0x59/0xa2/0xeb/0x34.
             */

            uint8_t tmpArr0[UART1_BUFFER_SIZE] = {
                '#',
                0xff,
            };
            tmpArr0[2] =
                pedal_state.jingle_array[255 - (pedal_state.play_motor_jingle--)]; // set accelerator value
                                                                                   // from jingle array
            tmpArr0[8] = frame_checksum(tmpArr0, UART1_BUFFER_SIZE);               // calculate checksum

            // send message
            pedal_uart_send(tmpArr0, 9);
        }
    }
}
#endif
