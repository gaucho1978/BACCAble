#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void exhaust_process(void) {
    if (settings_state.qv_exhaust_flap_function_enabled) {

        // Timeout management for chinese valves radiocontrol
        if (comfort_state.exhaust_valve_mosfet_command_time) {
            if (currentTime - comfort_state.exhaust_valve_mosfet_command_time >
                1000) {                                       // if timeout has passed
                HAL_GPIO_WritePin(Q10mosfet, GPIO_PIN_RESET); // open the transistor (release the button)
                HAL_GPIO_WritePin(Q11mosfet, GPIO_PIN_RESET); // open the transistor (release the button)
                comfort_state.exhaust_valve_mosfet_command_time = 0;
            }
        }

        // if a request to press the button on the radiocontrol was made
        if (comfort_state.chinese_exhaust_valve_request) { // O=open, C=close, 0x00=none
            HAL_GPIO_WritePin(Q10mosfet_Port,
                              (comfort_state.chinese_exhaust_valve_request == 'O') ? Q10mosfet_Pin
                                                                                   : Q11mosfet_Pin,
                              GPIO_PIN_SET); // close transistor (press button)
            comfort_state.chinese_valve_is_opened =
                (comfort_state.chinese_exhaust_valve_request == 'O'); // store valves status
            comfort_state.exhaust_valve_mosfet_command_time =
                currentTime;                                 // store time when button was pressed
            comfort_state.chinese_exhaust_valve_request = 0; // mark the request as executed
        }

        // if valve was open and engine had been shutted off
        if (comfort_state.chinese_valve_is_opened && telemetry_state.current_rpm_speed == 0) {
            HAL_GPIO_WritePin(Q11mosfet, GPIO_PIN_SET); // close transistor
            comfort_state.exhaust_valve_mosfet_command_time =
                currentTime;                           // store time when button was pressed
            comfort_state.chinese_valve_is_opened = 0; // update valves status
        }

        switch (comfort_state.force_q_vexhaust_valve_opened) {
        case 1: // send connection request
        case 2: // send presence
        case 3: // overwrite param
        case 4: // return control to ECU
            if (currentTime - comfort_state.last_sent_q_vexhaust_valve_msg_time >
                500) { // each 500msec send a message
                if (telemetry_state.current_rpm_speed == 0)
                    comfort_state.force_q_vexhaust_valve_opened = 4; // return control to ecu
                status_led_activity();
                // status_led_error();
                can_tx(
                    &comfort_state
                         .force_q_vexhaust_valve_msg_header[comfort_state.force_q_vexhaust_valve_opened - 1],
                    comfort_state
                        .force_q_vexhaust_valve_msg_data[comfort_state.force_q_vexhaust_valve_opened -
                                                         1]); // send connect message
                comfort_state.last_sent_q_vexhaust_valve_msg_time = currentTime;
                switch (comfort_state.force_q_vexhaust_valve_opened) {
                case 1:                                            // connection request was sent
                case 2:                                            // presence was sent
                    comfort_state.force_q_vexhaust_valve_opened++; // prepare to send next
                    break;
                case 3:                                            // param overwrite was sent
                    comfort_state.force_q_vexhaust_valve_opened--; // prepare to send tester presence
                    break;
                case 4:                                              // return control to ECU was sent
                    comfort_state.force_q_vexhaust_valve_opened = 0; // sequence end
                    break;
                default: // we will never come here
                    break;
                }
            }
            break;
        default: // do nothing
            break;
        }
    }
}
#endif
