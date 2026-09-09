#include "vehicle/standard_frames.h"
#include "app/powertrain.h"
/* CAN ID 0x00000192. */
void vehicle_handle_gear_lever(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 1)
        return;

#if defined(BACCABLE_C1)
    // if release button was pressed twice, toggle QV exhaust valve
    if ((frame_data[0] & 0b00001100) == 0x04) { // release button is pressed
        if (comfort_state.releasebutton_press_begin_time ==
            0) { // if button was not pressed, and now it is pressed
            comfort_state.releasebutton_press_begin_time =
                currentTime; // save current time it was pressed (press begin)
            comfort_state.number_of_release_button_clicks++;
            if (comfort_state.number_of_release_button_clicks == 1)
                comfort_state.releasebutton_first_click_time = currentTime;
        }

        if ((currentTime - comfort_state.releasebutton_press_begin_time) >
            2000) { // if pressed since 2 seconds
            status_led_activity();
            comfort_state.releasebutton_press_begin_time = 0; // reset the timer, like if it was not pressed
            comfort_state.number_of_release_button_clicks = 0;
        }

    } else {
        comfort_state.releasebutton_press_begin_time =
            0; // use this value to remember that button is not pressed
        if (currentTime - comfort_state.releasebutton_first_click_time >
            1000) { // if more than 1 second is passed since first button click
            comfort_state.number_of_release_button_clicks =
                0; // reset also the counter of the number of consecutive clics :-)
        }
        if (comfort_state.number_of_release_button_clicks >= 2) { // if double click
            comfort_state.number_of_release_button_clicks = 0;    // ensure we don't return here :-)
            status_led_activity();
            // execute action :-)
            if (settings_state.qv_exhaust_flap_function_enabled) {
                if (comfort_state.force_q_vexhaust_valve_opened == 0) { // if valves are closed
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
            }
        }
    }
    // P button, located on the gear shift lever, is on byte 0, bit 0 and 1 (3=failure, 2=pressed, 1=not
    // pressed,0=init) gear shift requested position is on byte 0 da bit 7 a bit 4. release button, located on
    // the gear shift lever, is on byte 0, bit 3 and 2 (0=not pressed, 1=pressed)
#endif
}

/* CAN ID 0x000002EE. */
void vehicle_handle_radio_buttons(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    // presente solo su BH can bus a 125kbps
    // this message contains the following radio buttons on the steering wheel:
    // radio right button is on byte 3 bit6 (1=button pressed)
    // radio left button on the steering wheel is on byte 3 bit4 (1=button pressed)
    // radio Voice command button is on byte 3 bit2 (1= button pressed)
    // phone call button is on byte3 bit0(1=button pressed)
    // volume  is on byte 4 (volume up increases the value, volume down reduces the value. once arrived to 255
    // restarts from 0 and under 0 goes to 255) volume change is on byte5 bit 7 and bit6 (1=volume was
    // increased rotation, 2=volume decreased rotation, 3=volume mute button press) (then reading the entire
    // byte we will see respectively, 0x40, 0x80,  0xC0) sample: uint8_t tmpCmd=frame_data[5] >>6; //1=volume
    // was increased rotation, 2=volume decreased rotation
}

/* CAN ID 0x00000358. */
void vehicle_handle_volume(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

    // should contain volume position (number of ticks in byte2, and direction in byte 2, bit 6 and 5)
}

/* CAN ID 0x000004B1. */
void vehicle_handle_start_stop(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_C1)
    if (comfort_state.request_to_disable_start_and_stop ==
        1) { // if requested, send message to simulate button press
        comfort_state.request_to_disable_start_and_stop = 0;
        comfort_state.start_and_stop_enabled = 0;                                           // done
        memcpy(&comfort_state.disable_start_and_stop_msg_data, frame_data, rx_header->DLC); // grab the
                                                                                            // message
        comfort_state.disable_start_and_stop_msg_header.DLC = rx_header->DLC;
        // set message data, byte 5, bits from 5 to 3 to binary 001.
        comfort_state.disable_start_and_stop_msg_data[5] =
            (comfort_state.disable_start_and_stop_msg_data[5] & 0b11000111) | (0x01 << 3);
        can_tx(&comfort_state.disable_start_and_stop_msg_header,
               comfort_state.disable_start_and_stop_msg_data);
        status_led_activity();
    }
#endif
    // Bonnet Status is on byte0 bit 4
    // driver door Fail status is on byte0 bit 3
    // FOB Search Request is on byte 0 bit from 2 to 1
    // Driver door status is on byte 0 bit 0
    // Passenger Door status is on byte1, bit 7
    // Left  Rear Door status is on byte 1 bit 6
    // Right Rear Door status is on byte 1 bit 5
    // Rear Hatch Status is on byte 1 bit 4
    // Rear Heated Window Status is on byte 1 bit 3
    // Front Heated Window Status is on byte 1 bit 2
    // Theft Alarm Status is on byte 2 from bit 6 to 4
    // Remote start Inhibit Status is on byte 2 from bit 3 to 0 and byte 3 from bit 7 to 6
    // Remote start Active status is on byte 3 bit 5
    // Battery state of function is on byte 3 from bit 4 to 0 and byte 4 bit 7
    // compressor Air Conditioner status is on byte 4 bit 5
    // Recalibration is on byte 4 bit 3
    // Exterior Rear Release Switch Status is on byte 4 bit 1
    // Start&Stop Pad1 is on byte 5 from bit 5 to 3 (value 1 enables and disables Start& stop)
}

/* CAN ID 0x000005A0. */
void vehicle_handle_lane_button(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 5)
        return;

#if defined(BACCABLE_BH)

    // Lane button press on some giulias (most recent?) is only on BH, byte4, bit 0. it is sent once per
    // second and on variation

    if ((frame_data[4] & 0x01) == 0x01) { // left stalk button was pressed (lane following indicator)
        if (chassis_state.lan_ebutton_press_begin_time ==
            0) { // if button was not pressed, and now it is pressed
            chassis_state.lan_ebutton_press_begin_time =
                currentTime; // save current time it was pressed (press begin)
            chassis_state.number_of_lane_button_clicks++;
            if (chassis_state.number_of_lane_button_clicks == 1)
                chassis_state.lan_ebutton_first_click_time = currentTime;
        }

        if ((currentTime - chassis_state.lan_ebutton_press_begin_time) > 2000) { // if pressed since 2 seconds
            // notify to C2, that LANE was pressed for more than 3 seconds -> request to toggle ESC/TC
            // status_led_error();
            if (settings_state.esc_tc_customizator_enabled) {
                uint8_t tmpArr1[2] = {C1BusID, C1cmdLaneSingleTap};
                board_uart_send(tmpArr1, 2);
                status_led_activity();
            }
            chassis_state.lan_ebutton_press_begin_time = 0; // reset the timer, like if it was not pressed
            chassis_state.number_of_lane_button_clicks = 0;
        }
    } else {
        chassis_state.lan_ebutton_press_begin_time =
            0; // use this value to remember that button is not pressed

        if (currentTime - chassis_state.lan_ebutton_first_click_time >
            1000) { // if more than 1 second is passed since first button click
            chassis_state.number_of_lane_button_clicks =
                0; // reset also the counter of the number of consecutive clics :-)
        }

        if (chassis_state.number_of_lane_button_clicks >= 2) { // if double click
            chassis_state.number_of_lane_button_clicks = 0;    // ensure we don't return here :-)
            // notify to C2 and C1, that LANE was pressed 2 times (double tap)
            uint8_t tmpArr1[2] = {C1_C2_BusID, C1_C2_cmdLaneDoubleTap};
            board_uart_send(tmpArr1, 2);
            status_led_activity();
        }
    }
#endif
}

/* CAN ID 0x000005A5. */
void vehicle_handle_cruise_control(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 1)
        return;

// cruise control ON/OFF status is on byte0 bit7 (0=disabled, 1=enabled)
#if defined(BACCABLE_C1)
    if ((frame_data[0] >> 7) == 1) {
        comfort_state.cruise_control_disabled = 0; // disable additional parameter menu commands
        // status_led_activity();
    } else {
        comfort_state.cruise_control_disabled = 1; // enable additional parameter menu commands
        // status_led_error();
    }
#endif
}

/* CAN ID 0x000005B0. */
void vehicle_handle_park_assist(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 2)
        return;

#if defined(BACCABLE_C2)

    if ((frame_data[1] == 0x20) &&
        (chassis_state.dyno_state_machine ==
         0xff)) { // park assist button was pressed and there is no dyno Start sequence in progress
        // HAS_buttonPressRequested=1;
        chassis_state.park_assist_button_press_count++;
        if (chassis_state.park_assist_button_press_count > 5) { // more or less 6 seconds
            chassis_state.park_assist_button_press_count = 0;   // reset the count
            chassis_toggle_dyno();
            // wait the feedback from ECU
        }
    } else {
        chassis_state.park_assist_button_press_count = 0; // reset the count assigning it zero
                                                          // HAS_buttonPressRequested=0;
    }
#endif
    // the park assistant button press event is on byte 1 bit 5 (1=pressed)
}

/* CAN ID 0x0000073C. */
void vehicle_handle_adaptive_cruise(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 8)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 8) {
        switch ((frame_data[7] >> 4) & 0x07) {
        case 0x00: // ACC is off
            // status_led_error();
            comfort_state.acc_disabled = 1; // enable additional parameter menu commands
            comfort_state.acc_engaged = 0;  // acc not engaged
            break;
        case 0x02: // acc engaged
            comfort_state.acc_engaged = 1;
            comfort_state.acc_disabled = 0; // disable additional parameter menu commands
            break;
        default:
            comfort_state.acc_disabled = 0; // disable additional parameter menu commands
            comfort_state.acc_engaged = 0;  // acc not engaged
        }
    }
#endif
    // contains status of ACC on byte 7, from bit 6 to 4 (0=disabled, 1=enabled, 2=engaged 3=engaged brake
    // only, 4=override, 5=cancel)
}
