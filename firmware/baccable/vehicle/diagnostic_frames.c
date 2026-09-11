#include "diagnostics/parameter_cache.h"
#include "diagnostics/fault_reader.h"
#include "vehicle/diagnostic_frames.h"
#include "diagnostics/parameter_request.h"

/* Deliver diagnostic replies and acknowledge supported vehicle-state changes. */
void vehicle_dispatch_diagnostic(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
#if defined(BACCABLE_C1)
    fault_reader_receive(rx_header, frame_data);
    if (security_state.immobilizer_enabled &&
        (telemetry_state.engine_on_since_more_than5seconds < 500)) { // if immo enabled and engine is off
        // if it is a message of connection to RFHUB, reset the connection periodically, but start the panic
        // alarm only once
        if (security_state.flood_the_bus == 0) { // if we are not flooding the bus
            uint8_t responseOffset =
                frame_data[0] >>
                4; // 0=single frame , 1=first fragmented frame 2=fragmented frame, 3=frame ack
            if ((rx_header->ExtId & 0xFFFFFFF0) == 0x18DAC7F0) { // if it is message from the thief
                if (responseOffset < 2 &&
                    rx_header->DLC >
                        (uint32_t)responseOffset +
                            1) { // we pass this if, in case of single frame and first fragmented frame
                    switch (frame_data[responseOffset + 1]) {
                    case 0x10: // diagnostic session
                    case 0x27: // security access
                    case 0x29: // authentication
                    case 0x3E: // tester presence
                    // case 0x1A: //??
                    case 0x2E:                            // write data by identifier
                    case 0x3D:                            // write memory by address
                        security_state.flood_the_bus = 1; // reset the RFHUB and start the alarm
                        break;
                    default:
                        break;
                    }
                }
            } else if ((rx_header->ExtId & 0xFFFFF0FF) == 0x18DAF0C7) { // if it is a reply from rfhub

                // time
                if (responseOffset < 2 &&
                    rx_header->DLC >
                        (uint32_t)responseOffset +
                            1) { // we pass this if, in case of single frame and first fragmented frame
                    switch (frame_data[responseOffset + 1]) {
                    case 0x50: // diagnostic session	//
                    case 0x67: // security access
                    case 0x69: // authentication
                    case 0x7E: // tester presence 	//
                    // case 0x1A: //??
                    case 0x6E:                            // write data by identifier
                    case 0x7D:                            // write memory by address
                        security_state.flood_the_bus = 1; // reset the RFHUB and start the alarm
                        break;
                    default:
                        break;
                    }
                }
            }
            if (security_state.flood_the_bus == 1) { // if we engaged the immobilizer
                security_state.flood_the_bus_start_time =
                    currentTime;       // set initial time we started to flood the bus
                status_led_activity(); // light a led
            }
        }
    } // end of immobilizer section

    parameter_request_receive(rx_header, frame_data);

    /*
    if(seatbeltAlarmDisabled==0xfe){ //if seatbelt status acquisition is in progress
            if(rx_header->ExtId==0x18DAF160){ //if received message comes from IPC
                    if (rx_header->DLC>=5){ //if at least 5 bytes
                            if(frame_data[1]==0x62){ //if param read reply successful
                                    if((frame_data[2]==0x55) && (frame_data[3]==0xA0 )){ //if param. 55A0
    (seat belt alarm status) seatbeltAlarmDisabled= !(frame_data[4]); //coherce to boolean and negate
                                    }
                            }
                    }
            }
    }
    */

    if ((diagnostics_state.seatbelt_alarm_disabled == 0x11) ||
        (diagnostics_state.seatbelt_alarm_disabled ==
         0x21)) { // if write param was sent (seatbelt disabling or enabling in progress)
        if (rx_header->ExtId == 0x18DAF160) { // if received message comes from IPC
            if (rx_header->DLC >= 4) {        // if at least 4 bytes
                if (frame_data[1] == 0x6F) {  // if write param reply successful
                    if ((frame_data[2] == 0x55) &&
                        (frame_data[3] == 0xA0)) { // if param wrote was 55A0 (en/dis seatbelt alam)
                        if (diagnostics_state.seatbelt_alarm_disabled == 0x11) { // if seatbelt disabling
                            diagnostics_state.seatbelt_alarm_disabled = 1;
                            parameter_cache_put(13, 1, currentTime); // seatbelt disabled
                        }
                        if (diagnostics_state.seatbelt_alarm_disabled == 0x21) { // if seatbelt enabling
                            diagnostics_state.seatbelt_alarm_disabled = 0;
                            parameter_cache_put(13, 0, currentTime); // seatbelt enabled
                        }
                    }
                }
            }
        }
    }

    if ((diagnostics_state.seatbelt_alarm_disabled == 0x10) ||
        (diagnostics_state.seatbelt_alarm_disabled ==
         0x20)) { // if diag session was sent (seatbelt disabling or enabling is in progress)
        if (rx_header->ExtId == 0x18DAF160) { // if received message comes from IPC
            if (rx_header->DLC >= 2) {        // if at least 2 bytes
                if (frame_data[1] == 0x50) {  // if diag session reply successful
                    // Send enable/disable seatbelt alarm message
                    diagnostics_state.uds_parameter_request_msg_header.ExtId = 0x18DA60F1;
                    diagnostics_state.uds_parameter_request_msg_header.DLC = 6;
                    diagnostics_state.uds_parameter_request_msg_data[0] = 0x05;
                    diagnostics_state.uds_parameter_request_msg_data[1] = 0x2F;
                    diagnostics_state.uds_parameter_request_msg_data[2] = 0x55;
                    diagnostics_state.uds_parameter_request_msg_data[3] = 0xA0;
                    diagnostics_state.uds_parameter_request_msg_data[4] = 0x03;

                    if (diagnostics_state.seatbelt_alarm_disabled == 0x10) { // if seatbelt disabling
                        diagnostics_state.uds_parameter_request_msg_data[5] =
                            0x00; // set byte to disable alarm
                    }
                    if (diagnostics_state.seatbelt_alarm_disabled == 0x20) {        // if seatbelt enabling
                        diagnostics_state.uds_parameter_request_msg_data[5] = 0x01; // send msg to enable
                                                                                    // alarm
                    }
                    // send message
                    can_tx(&diagnostics_state.uds_parameter_request_msg_header,
                           diagnostics_state
                               .uds_parameter_request_msg_data); // transmit the diag session request

                    diagnostics_state.seatbelt_alarm_disabled++; // record that operation was executed
                    diagnostics_state.seatbelt_alarm_status_request_time = currentTime;
                }
            }
        }
    }

    if (settings_state.route_msg_enabled == 1) {
        if (rx_header->ExtId ==
            0x18DABAF1) { // if route request and dashboard menu not shown to avoid conflicts
            if (rx_header->DLC >= 7) {
                diagnostics_state.route_std_id_msg =
                    !(frame_data[2] >> 4); // standard or extended msgID route request
                diagnostics_state.route_offset = (frame_data[2] & 0x0F); // offset from which start to copy
                diagnostics_state.route_msg_data[2] = frame_data[2];     // copy in the response

                diagnostics_state.route_msg_id = ((uint32_t)frame_data[3] << 24) | // MSB
                                                 ((uint32_t)frame_data[4] << 16) |
                                                 ((uint32_t)frame_data[5] << 8) |
                                                 ((uint32_t)frame_data[6]); // LSB

                status_led_activity();
            }
        }

        if (dashboard_state.baccable_dashboard_menu_visible)
            diagnostics_state.route_std_id_msg =
                0xff; // disables the route request, to avoid conflicts with show params functionality

        if (diagnostics_state.route_std_id_msg == 0) { // if we have to do it (ext id route request)
            if (rx_header->ExtId == diagnostics_state.route_msg_id) { // received msg to route
                diagnostics_state.route_std_id_msg =
                    0xFF; // set this to disable the request. only one message is routed to avoid bus flood
                if (diagnostics_state.route_offset < rx_header->DLC) { // send only if offset is correct
                    uint8_t sizeToCopy = 5;
                    if ((rx_header->DLC - diagnostics_state.route_offset) < sizeToCopy)
                        sizeToCopy = rx_header->DLC - diagnostics_state.route_offset;
                    memcpy(&diagnostics_state.route_msg_data[3], &frame_data[diagnostics_state.route_offset],
                           sizeToCopy);
                    if (sizeToCopy < 5)
                        memset(&diagnostics_state.route_msg_data[3 + sizeToCopy], 0x00, 5 - sizeToCopy);

                    can_tx(&diagnostics_state.route_msg_header, diagnostics_state.route_msg_data);
                    status_led_activity();
                }
            }
        }
    }
#endif // end define

#if defined(BACCABLE_C2)
    if (rx_header->ExtId == 0x18DAF128 &&
        chassis_state.dyno_state_machine !=
            0xff) { // if message from ABS ECU and Dyno state machine is in progress
        if (chassis_state.dyno_state_machine == 0 &&
            rx_header->DLC >= 3) { // we received a reply to diagnostic session request msg
            if (frame_data[0] == 0x06 && frame_data[1] == 0x50 &&
                frame_data[2] == 0x03) {            // if request was successful
                chassis_state.dyno_state_machine++; // send dyno sts msg
            }
        }
        if (chassis_state.dyno_state_machine == 1 &&
            rx_header->DLC >= 5) { // we received a reply to dyno status msg
            if (frame_data[0] == 0x05 && frame_data[1] == 0x62 && frame_data[2] == 0x30 &&
                frame_data[3] == 0x02) {                 // if request was successful
                chassis_state.dyno_state_machine++;      // send dyno disable
                if (frame_data[4] == 0x00) {             // if it is disabled, we shall enable it
                    chassis_state.dyno_mode_enabled = 0; // refresh current status
                    chassis_state.dyno_state_machine++;  // send dyno enable
                } else {                                 // it is enabled, we shall disable it
                    chassis_state.dyno_mode_enabled = 1; // refresh current status
                }
            }
        }
        if (chassis_state.dyno_state_machine == 2 &&
            rx_header->DLC >= 4) { // we received a reply to dyno disable msg
            if (frame_data[0] == 0x03 && frame_data[1] == 0x6E && frame_data[2] == 0x30 &&
                frame_data[3] == 0x02) {                 // if request was successful
                chassis_state.dyno_mode_enabled = 0;     // success change complete
                chassis_state.dyno_state_machine = 0xff; // disable state machine

                // send message to master to inform about the status of Dyno
                uint8_t tmpArr2[2] = {C1BusID, C1cmdDynoNotActive};
                if (chassis_state.dyno_mode_enabled)
                    tmpArr2[1] = C1cmdDynoActive;
                board_uart_send(tmpArr2, 2);

                status_led_activity();
            }
        }
        if (chassis_state.dyno_state_machine == 3 &&
            rx_header->DLC >= 4) { // we received a reply to dyno enable msg
            if (frame_data[0] == 0x03 && frame_data[1] == 0x6E && frame_data[2] == 0x30 &&
                frame_data[3] == 0x02) {             // if request was successful
                chassis_state.dyno_mode_enabled = 1; // success change complete

                chassis_state.dyno_state_machine = 0xff; // disable state machine

                // send message to master to inform about the status of Dyno
                uint8_t tmpArr2[2] = {C1BusID, C1cmdDynoNotActive};
                if (chassis_state.dyno_mode_enabled)
                    tmpArr2[1] = C1cmdDynoActive;
                board_uart_send(tmpArr2, 2);

                status_led_activity();
            }
        }

        if (chassis_state.dyno_state_machine != 0xff && rx_header->DLC >= 3) { // in any case
            if (frame_data[1] == 0x7F) {                 // if request refused, abort all
                chassis_state.dyno_state_machine = 0xff; // disable state machine

                // send message to master to inform about the status of Dyno
                uint8_t tmpArr2[2] = {C1BusID, C1cmdDynoNotActive};
                if (chassis_state.dyno_mode_enabled)
                    tmpArr2[1] = C1cmdDynoActive;
                board_uart_send(tmpArr2, 2);

                status_led_activity();
            }
        }
        if (chassis_state.dyno_state_machine != 0xff) { // if we are running, send next message
            chassis_state.dyno_msg_header.DLC =
                chassis_state.dyno_msg_data[chassis_state.dyno_state_machine][0] + 1;
            can_tx(&chassis_state.dyno_msg_header,
                   chassis_state
                       .dyno_msg_data[chassis_state.dyno_state_machine]); // add to the transmission queue
            status_led_activity();
            chassis_state.dyno_state_machine_last_update_time = currentTime; // save last time it was updated
        }
    }
#endif
}
