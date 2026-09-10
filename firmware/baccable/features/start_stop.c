#include "app/powertrain.h"
#include "features/periodic.h"
#if defined(BACCABLE_C1)

/* Maintain the selected automatic engine-stop behavior. */
void start_stop_process(void) {
    if (settings_state.smart_disable_start_stop_enabled) {
        if (comfort_state.start_and_stop_enabled) {
            if (currentTime > 10000) { // first 10 seconds don't do anything to avoid to disturb other startup
                                       // functions or immobilizer
                if (telemetry_state.engine_on_since_more_than5seconds >=
                    500) { // if motor is on since at least 5 seconds

                    if (comfort_state.start_andstop_car_status ==
                        0) { // if start & stop was found disabled in car, we don't need to do anything. Avoid
                             // to enter here; We enter here in example if board is switched when the car is
                             // running and S&S was still manually disabled by the pilot
                        comfort_state.start_and_stop_enabled = 0;
                        comfort_state.request_to_disable_start_and_stop = 0;
                    } else {
                        if (comfort_state.last_time_start_andstop_disabler_button_pressed ==
                            0) { // first time we arrive here, go inside
                            comfort_state.request_to_disable_start_and_stop = 1;
                        }
                    }
                }
            }
        }
    }

    /*
    if(seatbeltAlarmDisabled==0xff ){ //if the status is unknown
            if(engineOnSinceMoreThan5seconds>=200){ //if motor is on since at least 2 seconds
                    //get the status of the seatbelt alarm

                    uds_parameter_request_msg_header.ExtId=0x18DA60F1;
                    uds_parameter_request_msg_data[0]=0x03;
                    uds_parameter_request_msg_data[1]=0x22;
                    uds_parameter_request_msg_data[2]=0x55;
                    uds_parameter_request_msg_data[3]=0xA0;
                    uds_parameter_request_msg_header.DLC=4;
                    can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the
    request seatbeltAlarmDisabled=0xfe; //status is in aquisition seatbeltAlarmStatusRequestTime=currentTime;
                    last_sent_uds_parameter_request_Time=currentTime;
            }
    }
    */
}
#endif
