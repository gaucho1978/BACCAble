#include "features/chassis.h"

#if defined(BACCABLE_C2)

/* Maintain enabled chassis features and report their state to the main board. */
void chassis_process() {
    if (chassis_state.dyno_state_machine != 0xff) { // if state machine in progress
        if (currentTime - chassis_state.dyno_state_machine_last_update_time > 4000) { // if older than 4 sec
            chassis_state.dyno_state_machine = 0xff; // timeout. stop any sequence
            // send message to master to inform about the status of Dyno
            uint8_t tmpArr2[2] = {C1BusID, C1cmdDynoNotActive};
            if (chassis_state.dyno_mode_enabled)
                tmpArr2[1] = C1cmdDynoActive;
            board_uart_send(tmpArr2, 2);
        }
    }

    if (chassis_state.dyno_mode_enabled) {
        // send tester presence each 450msec if dyno is enabled
        if (currentTime - chassis_state.last_sent_tester_presence_msg_time >
            500) { // enter here once each 500msec
            chassis_state.last_sent_tester_presence_msg_time = currentTime;
            chassis_state.dyno_msg_header.DLC = chassis_state.dyno_msg_data[4][0] + 1;
            can_tx(&chassis_state.dyno_msg_header,
                   chassis_state.dyno_msg_data[4]); // add to the transmission queue
        }
    }
    if (chassis_state.front_brake_forced == 255) { // request to disable Front brake
        chassis_state.front_brake_forced = 0;
        // just reply to C1 baccable
        uint8_t tmpArr[2] = {C1BusID, C1cmdNormalFrontBrake};
        board_uart_send(tmpArr, 2);
        can_tx(&chassis_state.rear_brake_msg_header[0],
               chassis_state.rear_brake_msg_data[0]); // send message to return control to ECU
    }

    if (chassis_state.front_brake_forced == 5) {
        chassis_state.front_brake_forced = 4;
        // send reply via serial line to C1 to inform that front brake is going to be forced
        uint8_t tmpArr[2] = {C1BusID, C1cmdForceFrontBrake};
        board_uart_send(tmpArr, 2);
    }

    if (chassis_state.front_brake_forced > 0) { // force front brake
        // we shall send msg sequence
        if (currentTime - chassis_state.last_sent_rear_brake_msg_time > 500) { // enter here once each 500msec
            chassis_state.last_sent_rear_brake_msg_time = currentTime;
            status_led_activity();
            can_tx(&chassis_state.rear_brake_msg_header[chassis_state.front_brake_forced - 1],
                   chassis_state.rear_brake_msg_data[chassis_state.front_brake_forced -
                                                     1]); // send message to force front brakes

            switch (chassis_state.front_brake_forced) {
            case 4:
            case 3:
                chassis_state.front_brake_forced--;
                break;
            case 2:
                chassis_state.front_brake_forced++;
                break;
            default:
                break;
            }
        }
    }
}

/* Request a change between normal and dyno operation. */
void chassis_toggle_dyno() {
    if (chassis_state.dyno_state_machine == 0xff) { // there is no dyno Start sequence in progress
        chassis_state.dyno_state_machine = 0;       // state machine
        chassis_state.stability_inverted = 0;       // do not change ESC and TC if dynomode is requested
        chassis_state.dyno_msg_header.DLC = chassis_state.dyno_msg_data[chassis_state.dyno_state_machine][0] +
                                            1; // length of DIAGNOSTIC SESSION msg
        can_tx(
            &chassis_state.dyno_msg_header,
            chassis_state.dyno_msg_data[chassis_state.dyno_state_machine]); // add to the transmission queue
        status_led_activity();
        chassis_state.dyno_state_machine_last_update_time = currentTime; // save last time seen
        // wait the feedback from ECU
    }
}

#endif
