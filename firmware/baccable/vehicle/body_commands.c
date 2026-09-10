#include "vehicle/body_commands.h"

/* Apply enabled locking, window and security behavior to body-control reports. */
void vehicle_handle_body_commands(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 8)
        return;
#if defined(BACCABLE_C1)
    if (rx_header->DLC == 8) {

        // avoid thief to simulate engine rotation. As soon as RFHUB tells ignition is not in RUN reset
        // engineOn status stored inside baccable
        if ((frame_data[0] & 0x0F) != 0x08)
            telemetry_state.engine_on_since_more_than5seconds = 0;

        if (settings_state.close_windows_with_door_lock) {
            switch (frame_data[2] >> 4) {
            case 0x01: // it is the message to lock the car.
                if (currentTime - comfort_state.door_close_time <
                    3000) { // if less than 3 sec from previous lock click
                    comfort_state.door_locks_requests_counter++; // if more clicks on the button after first
                                                                 // were performed, count them
                } else {
                    // abort the action
                    comfort_state.door_locks_requests_counter = 0;
                    comfort_state.close_windows_request = 0;

                    // but...
                    if (settings_state.close_windows_with_door_lock ==
                        1) { // if close Windows 1 is selected in setup menu
                        comfort_state.door_locks_requests_counter =
                            1; // when door closes, we shall close windows
                    }
                }

                if (comfort_state.door_locks_requests_counter >= 1) { // if more clicks
                    comfort_state.rf_fob_number =
                        frame_data[1] &
                        0x1E; // store fob id (the real id is obtainable by shifting by 1 bit to the right,
                              // but this value is better, in order to be used in the next message)
                    comfort_state.rf_requestor =
                        frame_data[2] &
                        0x0E; // store requestor ID (real ID is obtainable by shifting by 1 bit to the right,
                              // but this value is better, in order to be used in the next message)
                    comfort_state.close_windows_request =
                        1; // within 3 second, we will send request to close windows
                }
                comfort_state.door_close_time = currentTime;
                break;
            case 0x04:                                   // it is the message to open the car
            case 0x03:                                   // it is the message to open the car (driver side)
                comfort_state.close_windows_request = 0; // interrupt the action, if in progress
                comfort_state.door_locks_requests_counter = 0;
                break;
            default:
            }

            switch (comfort_state.close_windows_request) {
            case 1: // we have to close the windows
                if (currentTime - comfort_state.door_close_time >
                    3500) { // if at least 3,5 seconds from door closure is passed
                    // send message to close the windows
                    frame_data[1] = comfort_state.rf_fob_number |
                                    0x01; // set proper key fob and set request to close all windows (0x01)
                    frame_data[2] = comfort_state.rf_requestor;                 // set requestor
                    frame_data[7] = frame_checksum(frame_data, rx_header->DLC); // update checksum
                    can_forward(rx_header, frame_data);                         // send msg

                    if (currentTime - comfort_state.door_close_time >
                        8000) { // after 4,5 seconds of windows movement, they should be closed
                        comfort_state.close_windows_request = 0;

                        if (comfort_state.door_locks_requests_counter >= 2) { // if windows Ajar is requested,
                            comfort_state.close_windows_request = 2;          // request the windows ajar
                        } else {
                            comfort_state.door_locks_requests_counter = 0;
                        }
                    }
                }

                break;
            case 2: // we have to set the windows Ajar
                // send message to open the windows
                if (currentTime - comfort_state.door_close_time > 8500) {
                    frame_data[1] = comfort_state.rf_fob_number; // set proper key fob
                    frame_data[2] =
                        0xB0 |
                        comfort_state.rf_requestor; // set request to open all windows (0xB0) and requestor
                    frame_data[7] = frame_checksum(frame_data, rx_header->DLC); // update checksum
                    can_forward(rx_header, frame_data);                         // send msg
                }

                if (currentTime - comfort_state.door_close_time >
                    8900) { // if at least 300msec from windows closed is passed, windows shoud be opened for
                            // at least 3 centimeters
                    comfort_state.close_windows_request = 0; // task completed
                    comfort_state.door_locks_requests_counter = 0;
                }

                break;
                // case 3: //we have to close the convertible top
                //	//send message to close convertible top

                // requestor 		frame_data[7] = frame_checksum(frame_data,rx_header->DLC); //update
                // checksum 		can_forward(rx_header, frame_data); //send msg

                // passed, the top shoud be closed 		closeWindowsRequest=0; //task completed

            default:
            }
        }

        if (settings_state.open_windows_with_door_lock) {
            switch (frame_data[2] >> 4) {
            case 0x01:                                  // it is the message to lock the car.
                comfort_state.open_windows_request = 0; // interrupt the action, if in progress
                comfort_state.door_unlocks_requests_counter = 0;
                break;
            case 0x04: // it is the message to open the car
            case 0x03: // it is the message to open the car
                if ((frame_data[2] & 0b00001110) !=
                    0x04) { // if requestor is not Passive Entry (door handle) (RfReq=0x2) (RfReq is on byte 2
                            // bit from 3 to 1).
                    if (currentTime - comfort_state.door_open_time <
                        3000) { // if less than 1 sec from previous unlock click
                        comfort_state.door_unlocks_requests_counter++; // if more clicks on the button after
                                                                       // first were performed, count them
                    } else {
                        // abort previous requests
                        comfort_state.door_unlocks_requests_counter = 0;
                        comfort_state.open_windows_request = 0;

                        // but...
                        if (settings_state.open_windows_with_door_lock ==
                            1) { // if open Windows 1 is selected in setup menu
                            comfort_state.door_unlocks_requests_counter =
                                1; // when door opens, we shall open windows
                        }
                    }
                }
                if (comfort_state.door_unlocks_requests_counter >=
                    1) { // if double click, windows opening is requested
                    comfort_state.rf_fob_number =
                        frame_data[1] &
                        0x1E; // store fob id (the real id is obtainable by shifting by 1 bit to the right,
                              // but this value is better, in order to be used in the next message)
                    comfort_state.rf_requestor =
                        frame_data[2] &
                        0x0E; // store requestor ID (real ID is obtainable by shifting by 1 bit to the right,
                              // but this value is better, in order to be used in the next message)
                    comfort_state.open_windows_request =
                        1; // within 1 second, we will send request to open windows
                }
                comfort_state.door_open_time = currentTime;
                break;
            default:
            }

            if (comfort_state.open_windows_request == 1) { // we have to open the windows
                if (currentTime - comfort_state.door_open_time >
                    3500) { // if at least 3,5 seconds from door opening is passed
                    // send message to open the windows
                    frame_data[1] = comfort_state.rf_fob_number; // set proper key fob
                    frame_data[2] =
                        0xB0 |
                        comfort_state.rf_requestor; // set request to open all windows (0xB0) and requestor
                    frame_data[7] = frame_checksum(frame_data, rx_header->DLC); // update checksum
                    can_forward(rx_header, frame_data);                         // send msg

                    if (currentTime - comfort_state.door_open_time >
                        8000) { // after 4,5 seconds of windows movement, they should be opened
                        comfort_state.open_windows_request = 0;
                        comfort_state.door_unlocks_requests_counter = 0;
                    }
                }
            }
        }

        /*
        if(function_remote_start_Enabled==1){
                if(frame_data[2]>>4==0x4){ //it is the message to open the car. THIS IS JUST FOR TEST!!!!!
                        RF_fob_number=frame_data[1] & 0x1E; //store fob id (the real id is obrainable by
        shifting by 1 bit to the right, but this value is better, in order to be used in the next message
                        //within 2 seconds, send remote start
                        doorOpenTime=currentTime;
                        engineRemoteStartRequest=3;
                }

                if(engineRemoteStartRequest){
                        //if(doorOpenTime+2000<currentTime){ //we have to start the engine
                        if(currentTime-doorOpenTime>2000){ //we have to start the engine
                                memcpy(&REMOTE_START_msg_data, frame_data, 8);
                                //REMOTE_START_msg_data[0]= (REMOTE_START_msg_data[0] & 0x0F ) | 0x80; //set
        custom key ignition status= custom key in ignition

                                REMOTE_START_msg_data[1]=RF_fob_number;//update the fob number
                                if(engineRemoteStartRequest==3){ //first message let's close the doors
                                        REMOTE_START_msg_data[2]=0x16;//update the function request (1= lock
        ports) and requestor (6=remote_start) doorOpenTime=currentTime-1000; //refresh time, so that we will
        wait 2 more seconds

                                }else{
                                        REMOTE_START_msg_data[2]=0x96;//update the function request (9= remote
        start) and requestor (6=remote_start)
                                        //REMOTE_START_msg_data[2]=0x92;//update the function request (9=
        remote start) and requestor (2=original keyless)
                                }
                                REMOTE_START_msg_data[6] ++; //update the counter
                                if(REMOTE_START_msg_data[6]>0x0F) REMOTE_START_msg_data[6]=0; //check the
        counter

                                REMOTE_START_msg_data[7] =
        frame_checksum(REMOTE_START_msg_data,REMOTE_START_msg_header.DLC); //update checksum
                                can_tx(&REMOTE_START_msg_header, REMOTE_START_msg_data); //send msg

                                //transmit one more message
                                //if(engineRemoteStartRequest==2){
                                //	REMOTE_START_msg_data[6] ++; //update the counter
                                //	if(REMOTE_START_msg_data[6]>0x0F) REMOTE_START_msg_data[6]=0; //check
        the counter
                                //	REMOTE_START_msg_data[7] =
        frame_checksum(REMOTE_START_msg_data,REMOTE_START_msg_header.DLC); //update checksum
                                //	can_tx(&REMOTE_START_msg_header, REMOTE_START_msg_data); //send msg
                                //	doorOpenTime=currentTime+5000; //set the trigger in order to enter
        next 10 seconds
                                //}

                                //if(engineRemoteStartRequest==1){

                                //	pressStartButton=1;
                                //}
                                status_led_activity();
                                if (engineRemoteStartRequest>0) engineRemoteStartRequest--; //avoid to return
        here after the required messages were sent
                        }
                }
        }
        */
    }
#endif
}
