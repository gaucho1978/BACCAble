#include "vehicle/standard_frames.h"
#include "app/powertrain.h"
/* CAN ID 0x000000FA. */
void vehicle_handle_brake_pedal(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_C2)
    // break pedal position byte0 bit 3 and 2 (10=pressed, 01=released)
#endif
}

/* CAN ID 0x000000FB. */
void vehicle_handle_engine_torque(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 4) {
        telemetry_state.torque = ((frame_data[2] & 0b01111111) << 4 | ((frame_data[3] >> 4) & 0b00001111));
        telemetry_state.torque = (telemetry_state.torque - 500);
    #ifdef TORQUE_CORRECTION_FACTOR
        telemetry_state.torque =
            telemetry_state.torque * TORQUE_CORRECTION_FACTOR; // custom correction factor in user_define.h
    #endif
    }

    if (chassis_state.launch_assist_enabled == 1) {
        if (telemetry_state.torque >= settings_state.launch_torque_threshold) {
            // send serial message to C2 baccable, to RELEASE front brakes
            uint8_t tmpArr3[2] = {C2BusID, C2cmdNormalFrontBrake};
            board_uart_send(tmpArr3, 2);
            chassis_state.launch_assist_enabled = 0; // ensure we do not return here
            // jump to statistics
            dashboard_state.dashboard_menu_indent_level = 1;
            dashboard_state.main_dashboard_page_index = 1; // params submenu

            dashboard_state.dashboard_page_index = parameter_page_find(0x1A); // 0-100km/h statistics
        }
    }
#endif
    // torque is on byte 2 from bit 6 to 0 and byte 3 from bit 7 to 4.
}

/* CAN ID 0x00000101. */
void vehicle_handle_vehicle_speed(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 3)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 3) {
        chassis_state.brake_intervention_acc_esc_asr = (frame_data[0] >> 5) & 0x1;

        // get vehicle speed
        telemetry_state.current_speed_km_h =
            (float)((((uint16_t)frame_data[0] << 11) & 0b0001111111111111) | ((uint16_t)frame_data[1] << 3) |
                    ((uint16_t)frame_data[2] >> 5)) /
            16;

        if (telemetry_state.current_speed_km_h == 0) {
            runtime_state.car_steady_counter++; // increase a counter
            if (runtime_state.car_steady_counter > 200)
                runtime_state.car_steady_counter = 200; // car is steady since 2 seconds
        } else {
            runtime_state.car_steady_counter = 0; // reset counter
        }

        // Execute Statistics
        if ((statistics_state.previous_speed_km_h < 0.0625) &&
            (telemetry_state.current_speed_km_h >= 0.0625)) { // we started. Let's count the time
            statistics_state.statistics_0_100_start_time = currentTime - 10;
            statistics_state.statistics_0_100_started = 1;
        }

        if (statistics_state.statistics_0_100_started) {
            statistics_state.chronometer_elapsed_time_0_100_km_h =
                ((float)(currentTime - statistics_state.statistics_0_100_start_time)) / 1000.0;

            if (telemetry_state.current_speed_km_h >= 100.0) { // target speed reached
                statistics_state.statistics_0_100_started = 0; // stop the time
                statistics_save_best();                        // save on flash, if it is a best time
            }

            if (statistics_state.chronometer_elapsed_time_0_100_km_h > 20.0) { // if it is missed
                statistics_state.statistics_0_100_started = 0;
            }
        }

        if ((statistics_state.previous_speed_km_h <= 100.0) &&
            (telemetry_state.current_speed_km_h > 100.0)) { // we started. Let's count the time
            statistics_state.statistics_100_200_start_time = currentTime - 10;
            statistics_state.statistics_100_200_started = 1;
        }

        if (statistics_state.statistics_100_200_started) {
            statistics_state.chronometer_elapsed_time_100_200_km_h =
                ((float)(currentTime - statistics_state.statistics_100_200_start_time)) / 1000.0;

            if (telemetry_state.current_speed_km_h >= 200.0) {   // target speed was reached
                statistics_state.statistics_100_200_started = 0; // stop the time
                statistics_save_best();                          // save on flash, if it is a best time
            }

            if (statistics_state.chronometer_elapsed_time_100_200_km_h > 40.0) { // if it is missed
                statistics_state.statistics_100_200_started = 0;
            }
        }

        statistics_state.previous_speed_km_h = telemetry_state.current_speed_km_h;
    }

#endif
}

/* CAN ID 0x000001F0. */
void vehicle_handle_clutch(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

    // clutch interlock is on byte 0 bit 7
    // clutch upstop is on byte0 bit 6
    // actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
    // analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.
}

/* CAN ID 0x0000001F7. */
void vehicle_handle_transmission_temperature(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 4) {
        telemetry_state.transmission_temperature =
            ((frame_data[2] & 0b00000001) << 5 | ((frame_data[3] >> 3) & 0b00011111));
        // status_led_activity();
    }
#endif
    // transmission temperature is on byte 2 bit 0 and byte 3 from bit 7 to bit 3
}

/* CAN ID 0x000001FC. */
void vehicle_handle_suspension(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    // received on C2 can bus
    // Rear Diff. Warning La. is on byte0 bit7
    // Rear Diff, Control Status is on byte0 bit6
    // Active Dumping Control Status (the suspensions) is on byte0 from bit 5 to 4 (0x0=Mid, 0x1=Soft,
    // 0x2=Firm [only on QV]) Rd. Asp. Ind. is on byte 0 from bit3 to 0 and byte 1 from bit 7 to 4 Active
    // Dumping Control Fail status is on byte 1 bit3 Aero. Fail Status is on byte 1 bit2 Front Aero. status is
    // on byte 1 bit1 to bit0 CDCM warning lamp is on byte 2 bit5
}

/* CAN ID 0x000002EF. */
void vehicle_handle_gear(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 1)
        return;
    // se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
#if defined(BACCABLE_C1)
    telemetry_state.current_gear = frame_data[0] & ~0xF;

    if (settings_state.led_strip_controller_enabled == 1) {
        display_state.scaled_color_set = gear_color(
            telemetry_state.current_gear); // prima di tutto azzeriamo i primi 4 bit meno significativi, poi
                                           // scala il dato con la funzione gear_color, per prepararlo per
                                           // l'invio alla classe vumeter
        led_strip_update(display_state.scaled_volume, display_state.scaled_color_set);
    }

    // if(function_regeneration_alert_enabled){
    //	if((frame_data[1] >>7)==1 && regenerationInProgress==0){ //if regeneration has just begun,
    //		uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
    //		board_uart_send(tmpArr3, 1);
    //	}
    // }
    // regenerationInProgress=frame_data[1] >>7; //DPF Regeneration mode is on byte 1 bit 7.

#endif

    // actual gear status is on byte 0 from bit 7 to 4 (0x0=neutral, 0x1 to 0x6=gear 1 to 6, 0x07=reverse
    // gear, 0x8 to 0xA=gear 7 to 9, 0xF=SNA) suggested gear status is on byte 0 from bit 3 to 0 DPF
    // Regeneration mode is on byte 1 bit 7. SAM info is on byte 1 from bit 3 to 0 stop start fault status is
    // on byte 2 bit 7
    //..
    // boost pressure indication is on byte 3 bit from 6 to 0 and byte 4  bit 7
}

/* CAN ID 0x000003E8. */
void vehicle_handle_body_gear(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;

#if defined(BACCABLE_BH)
    // gearEngaged is on byte 3 bit 3 to 0
    // actualGearGSI is on byte 3 bit 7 to 4
    telemetry_state.current_gear = frame_data[3] & 0x0F;
#endif
}

/* CAN ID 0x00000412. */
void vehicle_handle_accelerator(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;
    // se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il
    // valore é >51 (sfrutto le info ottenute sniffando)
#if defined(BACCABLE_C1)
    if (settings_state.led_strip_controller_enabled == 1) {
        if ((rx_header->DLC == 5) && (frame_data[3] >= 51)) {
            comfort_state.time_since_last_received_accelerator_message = currentTime;
            display_state.leds_strip_is_on = 1;
            display_state.scaled_volume = accelerator_percent(
                frame_data[3]); // prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
            led_strip_update(display_state.scaled_volume, display_state.scaled_color_set);
        }
    }
#endif
}

/* CAN ID 0x0000041A. */
void vehicle_handle_battery(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 6)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 6) {
        telemetry_state.battery_state_of_charge =
            (frame_data[1] & 0b01111111); // set Most Significant Bit to zero
        telemetry_state.battery_current = (frame_data[4] << 4 | (frame_data[5] >> 4));
    }
#endif
    // battery state of charge is on byte 1 from bit 6 to 0 (Percentage)
    // battery current (A) is on byte 4 and in byte 5 from bit 7 to bit 4
}

/* CAN ID 0x00000420. */
void vehicle_handle_battery_aux(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 6) {
        // batteryStateOfCharge=frame_data[0];
    }
#endif
}

/* CAN ID 0x000004B2. */
void vehicle_handle_oil(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;

#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 4) {
        telemetry_state.oil_pressure =
            ((frame_data[0] & 0b00000001) << 7 | ((frame_data[1] >> 1) & 0b01111111));
        telemetry_state.oil_temperature =
            ((frame_data[2] & 0b00111111) << 2 | ((frame_data[3] >> 6) & 0b00000011));
    }
#endif
    // engine oil level is in byte 0 from bit 7 to 3.
    // engine oil over fill status is on byte 0, bit 2.
    // engine oil min. is on byte 0 bit 1
    // engine oil pressure is on byte 0, bit 0 and on byte 1 from bit 7 to 1. (bar)
    // power mode status is on byte 1 bit 0 and on byte 2 bit 7.
    // engine water level is on byte 2 bit 6.
    // engine oil temperature is on byte 2 from bit 5 to 0 and on byte 3 from bit 7 to 6.
    // engine oil temperature warning light is on byte 3 bit 5.
}

/* CAN ID 0x000004B4. */
void vehicle_handle_chassis_aux(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_C2)
#endif
}

/* CAN ID 0x000005AE. */
void vehicle_handle_regeneration(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 6)
        return;

#if defined(BACCABLE_C1)
    // this message is directed to IPC once per second. DPF status is on byte 4 bit 2. (1=dirty, 0=clean)
    if (rx_header->DLC >= 6) {
        telemetry_state.diesel_engine_regeneration_mode = (frame_data[5] >> 2) & 0b00000111; // byte 5 bit 4-2

        if (settings_state.regeneration_alert_enabled) {    // if  function was enabled in setup menu
            if (telemetry_state.regeneration_in_progress) { // if regeneration is in progress
                if ((frame_data[4] & 0x04) == 0) {          // if the message needs to be changed
    #ifdef DPF_REGEN_VISUAL_ALERT
                    // change message and send it again
                    // memcpy(STATUS_ECM_msg_data, frame_data, rx_header->DLC); //copy message
                    // STATUS_ECM_msg_data[4] |= 0x04; //DPF Dirty (bit 2) set to ON
                    // STATUS_ECM_msg_header.DLC=rx_header->DLC;
                    // can_tx(&STATUS_ECM_msg_header, STATUS_ECM_msg_data); //send msg
                    frame_data[4] |= 0x04;              // DPF Dirty (bit 2) set to ON
                    can_forward(rx_header, frame_data); // send msg
    #endif
                    status_led_activity();
                }
            }

            if ((telemetry_state.diesel_engine_regeneration_mode == 2) &&
                (telemetry_state.regeneration_in_progress == 0)) { // if regeneration has just begun,
    #ifdef DPF_REGEN_SOUND_ALERT
                uint8_t tmpArr3[1] = {BhBusChimeRequest}; // play sound
                board_uart_send(tmpArr3, 1);
    #endif
                telemetry_state.regeneration_in_progress = 1;
                telemetry_state.loops_from_regeneration_ended = 0;
            }
            if ((telemetry_state.diesel_engine_regeneration_mode == 0) &&
                (telemetry_state.regeneration_in_progress == 1)) { // if regeneration has ended,
                telemetry_state.loops_from_regeneration_ended++;
                if (telemetry_state.loops_from_regeneration_ended > 10) {
                    telemetry_state.regeneration_in_progress = 0;
                    telemetry_state.loops_from_regeneration_ended = 0;
                }

            } else {
                telemetry_state.loops_from_regeneration_ended = 0;
            }
        }
    }
#endif
}

/* CAN ID 0x0000073A. */
void vehicle_handle_clock(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

    // contains current date from byte 0 to 7.
    // Hex values are used as characters in example 0x21 0x02 0x26 0x01 0x20 0x 25 represents
    // the date h21 minutes 02 day 26 month 01 year 2025.
    // last two bytes of the message are 00 00.
}
