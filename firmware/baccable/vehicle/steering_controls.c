#include "platform/debug.h"
#include "vehicle/steering_controls.h"

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

    if (comfort_state.cruise_control_disabled &&
        comfort_state.acc_disabled) { // if we are allowed to press buttons, use them in baccable menu
        switch (frame_data[0]) {
        case 0x18: // if cruise control speed reduction button was pressed, user wants to see next page
            if (comfort_state.wheel_pressed_button_id == 0x10 &&
                dashboard_state.baccable_dashboard_menu_visible) { // if button released, use pressed button
                comfort_state.wheel_pressed_button_id = 0x18;      // avoid to return here
                if (dashboard_state.commands_menu_enabled) {
                    switch (dashboard_state.dashboard_menu_indent_level) {
                    case 0:                                             // main menu
                        dashboard_state.main_dashboard_page_index += 1; // set next page

                        if (settings_state.read_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 2)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.clear_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 3)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.dyno_mode_master_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 5)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.esc_tc_customizator_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 6)
                                dashboard_state.main_dashboard_page_index++;
                        }
                        if (settings_state.front_brake_forcer_master == 0) {
                            if (dashboard_state.main_dashboard_page_index == 7)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.awd_disabler_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 8)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.has_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 11)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.qv_exhaust_flap_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 12)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (dashboard_state.main_dashboard_page_index >=
                            dashboard_state.dashboard_main_menu_array_len)
                            dashboard_state.main_dashboard_page_index = 0; // make a rotative menu
                        // status_led_activity();
                        dashboard_send_main(); // send dashboard page via usb
                        break;
                    case 1:
                        if (dashboard_state.main_dashboard_page_index == 1) { // we are in show params submenu
                            dashboard_state.dashboard_page_index += 1;        // set next page
                            if (settings_state.is_diesel_enabled == 1) {
                                if (dashboard_state.dashboard_page_index >= diesel_page_count)
                                    dashboard_state.dashboard_page_index = 0; // make a rotative menu
                            } else {
                                if (dashboard_state.dashboard_page_index >= gasoline_page_count)
                                    dashboard_state.dashboard_page_index = 0; // make a rotative menu
                            }

                            dashboard_state.dashboard_page_index =
                                parameter_page_next(dashboard_state.dashboard_page_index);
                            // status_led_activity();
                            displayed_parameter_values[0] = NAN; // zeroize params
                            displayed_parameter_values[1] = NAN; // zeroize params

                            dashboard_send_values(); // send dashboard page to BH
                        }
                        if (dashboard_state.main_dashboard_page_index == 9) { // we are in setup menu
                            setup_move_page(1);
                            dashboard_send_setup(); // send
                        }
                        if (dashboard_state.main_dashboard_page_index == 10) { // we are in params setup menu
                            parameter_setup_page_index += 1;                   // set next page
                            if (parameter_setup_page_index > parameter_page_count)
                                parameter_setup_page_index = 0; // make a rotative menu
                            dashboard_send_parameter_setup();   // send
                        }
                        break;
                    default:
                        break; // unexpected
                    }
                }
            }
            break;
        case 0x20: // if cruise control speed strong reduction button was pressed, user wants to jump 10 pages
                   // forward
            if (comfort_state.wheel_pressed_button_id == 0x18 &&
                dashboard_state.baccable_dashboard_menu_visible) { // if button released, use pressed button
                comfort_state.wheel_pressed_button_id = 0x20;      // avoid to return here
                if (dashboard_state.commands_menu_enabled) {
                    switch (dashboard_state.dashboard_menu_indent_level) {
                    case 0:                                             // main menu
                        dashboard_state.main_dashboard_page_index += 1; // set next page
                        if (settings_state.read_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 2)
                                dashboard_state.main_dashboard_page_index++;
                        }
                        if (settings_state.clear_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 3)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.dyno_mode_master_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 5)
                                dashboard_state.main_dashboard_page_index++;
                        }
                        if (settings_state.esc_tc_customizator_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 6)
                                dashboard_state.main_dashboard_page_index++;
                        }
                        if (settings_state.front_brake_forcer_master == 0) {
                            if (dashboard_state.main_dashboard_page_index == 7)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.awd_disabler_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 8)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.has_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 11)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (settings_state.qv_exhaust_flap_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 12)
                                dashboard_state.main_dashboard_page_index++;
                        }

                        if (dashboard_state.main_dashboard_page_index >=
                            dashboard_state.dashboard_main_menu_array_len)
                            dashboard_state.main_dashboard_page_index = 0; // make a rotative menu
                        // status_led_activity();
                        dashboard_send_main(); // send dashboard page to BH
                        break;
                    case 1:
                        if (dashboard_state.main_dashboard_page_index == 1) { // we are in show params submenu
                            dashboard_state.dashboard_page_index +=
                                10; // set 10 pages forward (+1 in gentle command)
                            if (settings_state.is_diesel_enabled == 1) {
                                if (dashboard_state.dashboard_page_index >= diesel_page_count)
                                    dashboard_state.dashboard_page_index = 0; // make a rotative menu
                            } else {
                                if (dashboard_state.dashboard_page_index >= gasoline_page_count)
                                    dashboard_state.dashboard_page_index = 0; // make a rotative menu
                            }
                            dashboard_state.dashboard_page_index =
                                parameter_page_next(dashboard_state.dashboard_page_index);
                            // status_led_activity();
                            displayed_parameter_values[0] = NAN; // zeroize params
                            displayed_parameter_values[1] = NAN; // zeroize params
                            dashboard_send_values();             // send dashboard page to BH
                        }
                        if (dashboard_state.main_dashboard_page_index == 9) { // we are in setup menu
                            setup_move_page(10);
                            dashboard_send_setup(); // send
                        }
                        if (dashboard_state.main_dashboard_page_index == 10) { // we are in params setup menu
                            parameter_setup_page_index += 10;                  // set next page
                            if (parameter_setup_page_index > parameter_page_count)
                                parameter_setup_page_index = 0; // make a rotative menu
                            // status_led_activity();
                            dashboard_send_parameter_setup(); // send
                        }
                        break;
                    default:
                        break; // unexpected
                    }
                }
            }

            break;
        case 0x08: // if cruise control speed increase button was pressed, user wants to see previous page
            if (comfort_state.wheel_pressed_button_id == 0x10 &&
                dashboard_state.baccable_dashboard_menu_visible) { // if button released, use pressed button
                comfort_state.wheel_pressed_button_id = 0x08;      // avoid to enter again here
                if (dashboard_state.commands_menu_enabled) {
                    switch (dashboard_state.dashboard_menu_indent_level) {
                    case 0:                                             // main menu
                        dashboard_state.main_dashboard_page_index -= 1; // set next page

                        if (dashboard_state.main_dashboard_page_index >=
                            dashboard_state.dashboard_main_menu_array_len)
                            dashboard_state.main_dashboard_page_index =
                                dashboard_state.dashboard_main_menu_array_len - 1; // make a rotative menu

                        if (settings_state.qv_exhaust_flap_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 12)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.has_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 11)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.awd_disabler_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 8)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.front_brake_forcer_master == 0) {
                            if (dashboard_state.main_dashboard_page_index == 7)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.esc_tc_customizator_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 6)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.dyno_mode_master_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 5)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.clear_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 3)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.read_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 2)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        // status_led_activity();
                        dashboard_send_main(); // send dashboard page via usb
                        break;
                    case 1:
                        if (dashboard_state.main_dashboard_page_index == 1) { // we are in show params submenu
                            dashboard_state.dashboard_page_index -= 1;        // set previous page
                            if (settings_state.is_diesel_enabled == 1) {
                                if (dashboard_state.dashboard_page_index >= diesel_page_count)
                                    dashboard_state.dashboard_page_index =
                                        diesel_page_count - 1; // make a rotative menu
                            } else {
                                if (dashboard_state.dashboard_page_index >= gasoline_page_count)
                                    dashboard_state.dashboard_page_index =
                                        gasoline_page_count - 1; // make a rotative menu
                            }
                            dashboard_state.dashboard_page_index =
                                parameter_page_previous(dashboard_state.dashboard_page_index);
                            // status_led_activity();
                            displayed_parameter_values[0] = NAN; // zeroize params
                            displayed_parameter_values[1] = NAN; // zeroize params
                            dashboard_send_values();             // send dashboard page to BH
                        }
                        if (dashboard_state.main_dashboard_page_index == 9) { // we are in setup menu
                            setup_move_page(-1);
                            dashboard_send_setup(); // send
                        }
                        if (dashboard_state.main_dashboard_page_index == 10) { // we are in params setup menu
                            parameter_setup_page_index -= 1;                   // set next page
                            if (parameter_setup_page_index > parameter_page_count)
                                parameter_setup_page_index = parameter_page_count; // make a rotative menu
                            // status_led_activity();
                            dashboard_send_parameter_setup(); // send
                        }
                        break;
                    default:
                        break; // unexpected
                    }
                }
            }
            break;
        case 0x00: // if cruise control speed strong increase button was pressed, user wants to jump 10 pages
                   // before
            if (comfort_state.wheel_pressed_button_id == 0x08 &&
                dashboard_state.baccable_dashboard_menu_visible) {
                comfort_state.wheel_pressed_button_id = 0x00; // avoid to return here
                if (dashboard_state.commands_menu_enabled) {
                    switch (dashboard_state.dashboard_menu_indent_level) {
                    case 0:                                             // main menu
                        dashboard_state.main_dashboard_page_index -= 1; // set next page
                        if (dashboard_state.main_dashboard_page_index >=
                            dashboard_state.dashboard_main_menu_array_len)
                            dashboard_state.main_dashboard_page_index =
                                dashboard_state.dashboard_main_menu_array_len - 1; // make a rotative menu

                        if (settings_state.qv_exhaust_flap_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 12)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.has_function_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 11)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (settings_state.awd_disabler_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 8)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.front_brake_forcer_master == 0) {
                            if (dashboard_state.main_dashboard_page_index == 7)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.esc_tc_customizator_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 6)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.dyno_mode_master_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 5)
                                dashboard_state.main_dashboard_page_index--;
                        }

                        if (dashboard_state.main_dashboard_page_index == 4)
                            dashboard_state.main_dashboard_page_index--;

                        if (settings_state.clear_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 3)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        if (settings_state.read_faults_enabled == 0) {
                            if (dashboard_state.main_dashboard_page_index == 2)
                                dashboard_state.main_dashboard_page_index--;
                        }
                        // status_led_activity();
                        dashboard_send_main(); // send dashboard page via usb
                        break;
                    case 1:
                        if (dashboard_state.main_dashboard_page_index == 1) { // we are in show params submenu
                            dashboard_state.dashboard_page_index -= 10;       // set 10 pages backward

                            if (settings_state.is_diesel_enabled == 1) {
                                if (dashboard_state.dashboard_page_index >= diesel_page_count)
                                    dashboard_state.dashboard_page_index = 0; // stay at zero.
                            } else {
                                if (dashboard_state.dashboard_page_index >= gasoline_page_count)
                                    dashboard_state.dashboard_page_index = 0; // stay at zero.
                            }
                            dashboard_state.dashboard_page_index =
                                parameter_page_previous(dashboard_state.dashboard_page_index);

                            // status_led_activity();
                            displayed_parameter_values[0] = NAN; // zeroize params
                            displayed_parameter_values[1] = NAN; // zeroize params
                            dashboard_send_values();             // send dashboard page to BH
                        }
                        if (dashboard_state.main_dashboard_page_index == 9) { // we are in setup menu
                            setup_move_page(-10);
                            dashboard_send_setup(); // send
                        }
                        if (dashboard_state.main_dashboard_page_index == 10) { // we are in params setup menu
                            parameter_setup_page_index -= 10;                  // set prev page
                            if (parameter_setup_page_index > parameter_page_count)
                                parameter_setup_page_index = 0; // make a rotative menu
                            // status_led_activity();
                            dashboard_send_parameter_setup(); // send
                        }
                        break;
                    default:
                        break; // unexpected
                    }
                }
            }
            break;
        case 0x10: // button released
            if (comfort_state.wheel_pressed_button_id == 0x89 &&
                dashboard_state.baccable_dashboard_menu_visible ==
                    1) { // we pressed RES for at least one instant, then we released before 2 seconds,
                         // therefore we want to enter inside dashboard menu (will work only if menu is
                         // visible)

                if (dashboard_state.dashboard_menu_indent_level == 0) {
                    switch (dashboard_state.main_dashboard_page_index) {
                    case 1: // show params
                        dashboard_state.dashboard_menu_indent_level++;
                        displayed_parameter_values[0] = NAN; // zeroize params
                        displayed_parameter_values[1] = NAN; // zeroize params
                        dashboard_send_values();
                        break;
                    case 2: // read faults
                        // To Be Done
                        break;
                    case 3: // clear faults
                        diagnostics_state.clear_faults_request = 255;
                        break;
                    case 4: // Immo
                        // nothing to do
                        break;
                    case 5: // toggle dyno status
                        // send request thu serial line
                        if (runtime_state.car_steady_counter >= 100) { // car is steady since at least 1
                                                                       // second
                            uint8_t tmpArr1[2] = {C2BusID, C2cmdtoggleDyno};
                            board_uart_send(tmpArr1, 2);
                        } else {
                            dashboard_state.print_stop_the_car = 2; // print message "stop the car"
                        }
                        break;
                    case 6: // toggle ESC/TC
                        // send request thu serial line
                        uint8_t tmpArr2[2] = {C2BusID, C2cmdtoggleEscTc};
                        board_uart_send(tmpArr2, 2);
                        break;
                    case 7: // toggle front brake

                        if (chassis_state.front_brake_forced > 0) { // toggle front brake
                            if (chassis_state.launch_assist_enabled ==
                                1) { // if assist is enabled (by default it is enabled)
                                chassis_state.launch_assist_enabled = 0; // disable assist
                            } else { // launch assist is not enabled, but brakes are forced
                                // send serial message to C2 baccable, to RELEASE front brakes
                                uint8_t tmpArr3[2] = {C2BusID, C2cmdNormalFrontBrake};
                                board_uart_send(tmpArr3, 2);
                            }
                        } else {
                            if (telemetry_state.current_speed_km_h == 0) {
                                if (!chassis_state.dyno_mode_enabled_on_master) {
                                    dashboard_state.print_enable_dyno = 2; // print message Enable DYNO
                                } else {
                                    /* This commented part is a test... not working now

                                    //if dyno function is enabled but dyno is not enabled, enable dyno
                                    if(function_dyno_mode_master_enabled){
                                            if(DynoModeEnabledOnMaster==0){
                                                    //send serial message to C2 baccable, to enable dyno
                                                    uint8_t tmpArr4[2]={C2BusID,C2cmdForceFrontBrake};
                                                    board_uart_send(tmpArr4, 2);
                                            }
                                    }

                                    //if 4wd function is enabled but 4wd is not disabled, disable 4wd
                                    if(function_4wd_disabler_enabled){
                                            if(_4wd_disabled==0){
                                                    //seend messages to disable	4wd
                                                    //TBDone.......
                                            }
                                    }
                                    */
                                    // send serial message to C2 baccable, to force front brakes
                                    uint8_t tmpArr5[2] = {C2BusID, C2cmdForceFrontBrake};
                                    board_uart_send(tmpArr5, 2);
                                }
                            } else {
                                dashboard_state.print_stop_the_car = 2; // print message "stop the car"
                            }
                        }

                        break;
                    case 8:                                   // toggle 4wd
                        if (chassis_state.awd_sequence > 0) { // toggle 4dw disable
                            chassis_state.awd_sequence = 0;
                            // update text
                            dashboard_state
                                .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] =
                                ' '; // enabled
                            dashboard_state
                                .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] =
                                'E';
                            dashboard_state
                                .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] =
                                'n';
                            dashboard_state.commands_menu_enabled = 1; // enable menu commands
                        } else {
                            if (runtime_state.car_steady_counter >=
                                100) { // car is steady since at least one second
                                chassis_state.awd_sequence = 4;
                                // update text
                                dashboard_state
                                    .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][4] =
                                    'D'; // disabled
                                dashboard_state
                                    .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][5] =
                                    'i';
                                dashboard_state
                                    .dashboard_main_menu_array[dashboard_state.main_dashboard_page_index][6] =
                                    's';
                                dashboard_state.commands_menu_enabled = 0; // disable menu commands
                            } else {
                                dashboard_state.print_stop_the_car = 2; // print message "stop the car"
                            }
                        }
                        break;
                    case 9:  // setup menu
                    case 10: // params setup menu
                        dashboard_state.dashboard_menu_indent_level++;
                        break;
                    case 11: // toggle HAS function
                        comfort_state.has_button_press_requested = 5;
                        // inform Slave baccable C2
                        uint8_t tmpArr3[2] = {C2BusID, C2cmdToggleHas};
                        board_uart_send(tmpArr3, 2);
                        break;
                    case 12:                                                    // toggle QV Exhaust Valve
                        if (comfort_state.force_q_vexhaust_valve_opened == 0) { // if valves are closed,
                            comfort_state.force_q_vexhaust_valve_opened = 1;    // start override sequence
                        } else {
                            comfort_state.force_q_vexhaust_valve_opened = 4; // return control to ecu
                        }

                        // equivalent activity for chinese valves
                        if (comfort_state.chinese_valve_is_opened == 0) {      // if valves are closed,
                            comfort_state.chinese_exhaust_valve_request = 'O'; // open request
                        } else {
                            comfort_state.chinese_exhaust_valve_request = 'C'; // close request
                        }

                        break;
                    case 13: // save Log to filesystem
                        // inform Slave baccable C2 and BH
                        uint8_t tmpArr4[2] = {C2_Bh_BusID, C2_Bh_cmdFunction_Save_Log_to_File};
                        board_uart_send(tmpArr4, 2);

                        break;
                    case 14: // reset statistics
                        statistics_reset();
                    default:
                        break;
                    }

                } else { // indent level >0
                    switch (dashboard_state.main_dashboard_page_index) {
                    case 9: // setup menu
                        setup_select_page(setup_dashboardPageIndex);
                        dashboard_send_setup();
                        break;
                    case 10: // PARAMS SETUP MENU
                        switch (parameter_setup_page_index) {
                        case 0: //{'S','A','V','E','&','E','X','I','T',},
                            visibility_save();
                            dashboard_state.dashboard_menu_indent_level = 0;
                            break;
                        default: // toggle hidden variable of current param
                            parameter_page_visibility[parameter_setup_page_index - 1] =
                                !parameter_page_visibility[parameter_setup_page_index - 1];
                            break;
                        }
                        break;
                    default:
                        // we want to return main menu
                        dashboard_state.dashboard_menu_indent_level = 0;
                        dashboard_send_main(); // print menu
                    }
                }
            }
            comfort_state.wheel_pressed_button_id = 0x10; // button released
            comfort_state.last_pressed_wheel_button_duration = 0;
            comfort_state.last_pressed_speed_up_wheel_button_duration = 0;
            break;
        case 0x90: // RES button was pressed
        case 0x50: // distance selector, used like RES, to manage the menu
    #ifndef HIDE_DASHBOARD_MENU
            comfort_state.last_pressed_wheel_button_duration++;

            if (comfort_state.wheel_pressed_button_id == 0x10 &&
                comfort_state.wheel_pressed_button_id != 0x90) { // we pressed RES for at least one instant
                comfort_state.wheel_pressed_button_id =
                    0x89; // avoid returning here until button is not released
            }
            if (comfort_state.wheel_pressed_button_id == 0x89 &&
                (comfort_state.last_pressed_wheel_button_duration >
                 50)) { // we pressed RES for around 2 seconds, therefore we want to enable/disable Baccable
                        // menu on dashboard
                comfort_state.wheel_pressed_button_id =
                    0x90; // avoid returning here until button is not released

                dashboard_state.baccable_dashboard_menu_visible =
                    !dashboard_state.baccable_dashboard_menu_visible; // toggle visualizazion of the menu

                // LOG("Dashboard vis: %d\r\n", baccableDashboardMenuVisible);

                if (!dashboard_state
                         .baccable_dashboard_menu_visible) { // if menu needs to be hidden, print spaces to
                                                             // clear the string on dashboard
                    dashboard_clear();
                } else {
                    // dashboardPageIndex=0; //reset the page, just to be sure to show initial Baccable print
                    // main_dashboardPageIndex=0; //shows initial baccable version
                    // dashboard_menu_indent_level=0;
                }
            }
    #endif

            break;
        case 0x12: // Cruise Control Disabled/Enabled
            break;
        default:
        }
    }

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
