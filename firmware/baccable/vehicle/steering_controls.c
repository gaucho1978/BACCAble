#include "platform/debug.h"
#include "vehicle/steering_controls.h"
#include "features/menu.h"

/* Interpret steering-wheel controls for the menu and enabled driving assistance. */
void vehicle_handle_steering_controls(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC != 3)
        return;
    // Button is pressed on left area of the wheel
    // These Buttons are detected only if the main panel of the car is on.

#if defined(BACCABLE_C2) || defined(BACCABLE_C1)
    if (comfort_state.has_button_press_requested) { // Has button press was requested
        comfort_state.has_button_press_requested--;
        if (frame_data[0] == 0x10) {              // if no button was pressed on cruise control pad
            frame_data[1] = frame_data[1] | 0x10; // simulate HAS button presses
            frame_data[2] = frame_checksum(frame_data, rx_header->DLC); // CRC
            can_forward(rx_header, frame_data); // send message to simulate RES button press
        }
    }
#endif

#if defined(BACCABLE_C1)

    // function ACC Virtual Pad

    if (settings_state.acc_virtual_pad_enabled == 1) {
        switch (frame_data[0]) {
        case 0x12: // CC on
            comfort_state.new_wheel_pressed_button_id = 0x12;
            memcpy(comfort_state.acc_msg_data, frame_data, rx_header->DLC);
            comfort_state.acc_msg_data[0] = 0x11; // ACC On
            comfort_state.acc_msg_data[1] =
                (comfort_state.acc_msg_data[1] & 0xF0) |
                (((comfort_state.acc_msg_data[1] & 0x0F) + 1) % 16); // increase the counter
            comfort_state.acc_msg_data[2] =
                frame_checksum(comfort_state.acc_msg_data, rx_header->DLC);    // update checksum
            can_tx(&comfort_state.acc_msg_header, comfort_state.acc_msg_data); // send msg
            status_led_activity();
            break;
        case 0x90: // RES pressed
            if (comfort_state.new_wheel_pressed_button_id == 0x10 && comfort_state.acc_engaged)
                comfort_state.acc_was_engaged_when_res_was_pressed =
                    1; // if begin to press button RES and ACC is Engaged, set
                       // ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED
            comfort_state.new_wheel_pressed_button_id = 0x90; // store the new RES button status (pressed)
            if (comfort_state.acc_engaged && comfort_state.acc_was_engaged_when_res_was_pressed) {
                // simulate the distance button press
                memcpy(comfort_state.acc_msg_data, frame_data, rx_header->DLC);
                comfort_state.acc_msg_data[0] = 0x50; // ACC distance change
                comfort_state.acc_msg_data[1] =
                    (comfort_state.acc_msg_data[1] & 0xF0) |
                    (((comfort_state.acc_msg_data[1] & 0x0F) + 1) % 16); // increase the counter
                comfort_state.acc_msg_data[2] =
                    frame_checksum(comfort_state.acc_msg_data, rx_header->DLC);    // update checksum
                can_tx(&comfort_state.acc_msg_header, comfort_state.acc_msg_data); // send msg
                status_led_activity();
            }
            break;
        case 0x10: // button released
            comfort_state.new_wheel_pressed_button_id =
                0x10; // button released (I use another variable to distinguish from the one used in show
                      // params function
            comfort_state.acc_was_engaged_when_res_was_pressed = 0;
            break;
        default:
        }
    }

    if (settings_state.acc_autostart) {
        if (comfort_state.acc_engaged) {
            if (runtime_state.car_steady_counter == 200 &&
                chassis_state
                    .brake_intervention_acc_esc_asr) { // if car is steady and brake is pressed by ACC
                if (frame_data[0] == 0x10) {           // if no button was pressed on cruise control pad
                    if (currentTime - runtime_state.last_sent_autostart_msg > 500) { // once each 1,5 seconds
                        frame_data[0] = 0x90;                                        // Res button press

                        if (settings_state.acc_autostart == 2) {
                            frame_data[0] = 0x08; // ACC gently up button press
                        }
                        frame_data[1] = (frame_data[1] & 0xF0) |
                                        (((frame_data[1] & 0x0F) + 1) % 16);        // increase the counter
                        frame_data[2] = frame_checksum(frame_data, rx_header->DLC); // update checksum

                        can_forward(rx_header, frame_data); // send message to simulate RES button press
                        frame_data[0] = 0x10; // restore value 10 to avoid unwanted behaviours with subsequent
                                              // pieces of code
                        // increase a counter
                        runtime_state.autostart_msg_counter++;
                        if (runtime_state.autostart_msg_counter >=
                            5) { // we are simulating a 100msec button press event
                            runtime_state.autostart_msg_counter = 0;
                            runtime_state.last_sent_autostart_msg = currentTime;
                        }
                    }
                }
            }
        }
    }

    menu_button(frame_data[0], comfort_state.cruise_control_disabled && comfort_state.acc_disabled);

    #ifndef PERMANENTLY_DISABLE_IMMO
    if (comfort_state.cruise_control_disabled &&
        comfort_state.acc_disabled) { // if we are allowed to use the buttons of the cruise control
        if (telemetry_state.current_rpm_speed > 400) { // if motor is on
            if (telemetry_state.current_gear == 0) {   // gear is neutral
                if ((frame_data[0] == 0x08) &&
                    ((comfort_state.wheel_pressed_button_id == 0x10) ||
                     (comfort_state.wheel_pressed_button_id ==
                      0x08))) { // user is pressing CC soft speed up button and it was previously released (or
                                // pressed by baccable menu up here)
                    comfort_state.last_pressed_speed_up_wheel_button_duration++;
                    if (comfort_state.last_pressed_speed_up_wheel_button_duration >
                        1267) { // around 30 seconds
                        // avoid to return here
                        comfort_state.wheel_pressed_button_id =
                            0xF8; // invent a new status to differentiate it from 0x08 used in baccable menu
                                  // few lines of code up here
                        comfort_state.last_pressed_speed_up_wheel_button_duration =
                            0; // unuseful here since it is done when button is released. just to be
                               // superstitious :-D.
                        security_state.immobilizer_enabled =
                            !security_state.immobilizer_enabled; // toggle immobilizer status
                        security_state.flood_the_bus =
                            0; // ensure to reset this even if probably it is not needed
                        if (settings_save() >
                            253) { // if we get error while permanently storeing the parameter on flash
                            security_state.immobilizer_enabled =
                                !security_state
                                     .immobilizer_enabled; // toggle immobilizer status to the original status
                                                           // and avoid to report the user anything
                            status_led_error();            // a problem occurred
                        } else {
                            status_led_activity(); // everything goes fine. change saved on flash
                            if (security_state.immobilizer_enabled) { // if immo enabled
                                dashboard_state.execute_dashboard_blinks =
                                    6; // blinks the dashboard brightness 3 times
                            } else {
                                dashboard_state.execute_dashboard_blinks =
                                    12; // blinks the dashboard brightness 6 times
                            }
                        }
                    }
                }
                if (frame_data[0] == 0x10) { // user released the button
                    comfort_state.last_pressed_speed_up_wheel_button_duration = 0;
                    comfort_state.wheel_pressed_button_id = 0x10; // button released
                }
            }
        }
    }
    #endif
#endif
}
