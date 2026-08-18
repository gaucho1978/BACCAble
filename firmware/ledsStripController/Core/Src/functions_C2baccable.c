/*
 * functions_C2baccable.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */
#include "functions_C2baccable.h"

#if defined(C2baccable)
	void C2PeriodicCheck(){
		if(DynoStateMachine!=0xff){ //if state machine in progress
			if(currentTime-DynoStateMachineLastUpdateTime> 4000){ //if older than 4 sec
				DynoStateMachine=0xff; //timeout. stop any sequence
				//send message to master to inform about the status of Dyno
				uint8_t tmpArr2[2]={C1BusID,C1cmdDynoNotActive};
				if(DynoModeEnabled) tmpArr2[1]=C1cmdDynoActive;
				addToUARTSendQueue(tmpArr2, 2);
			}
		}


		if(DynoModeEnabled){
			//send tester presence each 450msec if dyno is enabled
			if(currentTime-last_sent_tester_presence_msg_time>500){ //enter here once each 500msec
				last_sent_tester_presence_msg_time=currentTime;
				DYNO_msg_header.DLC=DYNO_msg_data[4][0]+1;
				can_tx(&DYNO_msg_header, DYNO_msg_data[4]); //add to the transmission queue

			}
		}
		if(front_brake_forced==255){ //request to disable Front brake
			front_brake_forced=0;
			//just reply to C1 baccable
			uint8_t tmpArr[2]={C1BusID,C1cmdNormalFrontBrake};
			addToUARTSendQueue(tmpArr, 2);
			can_tx(&rearBrakeMsgHeader[0], rearBrakeMsgData[0]); //send message to return control to ECU


		}

		if(front_brake_forced==5){
			front_brake_forced=4;
			//send reply via serial line to C1 to inform that front brake is going to be forced
			uint8_t tmpArr[2]={C1BusID,C1cmdForceFrontBrake};
			addToUARTSendQueue(tmpArr, 2);
		}

		if(front_brake_forced>0){ //force front brake
			//we shall send msg sequence
			if(currentTime-last_sent_rear_brake_msg_time>500){ //enter here once each 500msec
				last_sent_rear_brake_msg_time=currentTime;
				onboardLed_blue_on();
				can_tx(&rearBrakeMsgHeader[front_brake_forced-1], rearBrakeMsgData[front_brake_forced-1]); //send message to force front brakes

				switch(front_brake_forced){
					case 4:
					case 3:
						front_brake_forced--;
						break;
					case 2:
						front_brake_forced++;
						break;
					default:
						break;
				}
			}
		}
/*
		// @netzmark PDC DISABLE code - begin
		#define pdcAutoDisableEnabled 1
		 //=========================================================================
		 //TOGGLE PDC SHOT followed with Release button simulation (to get faster reaction)
		 // this made intentionally NOT in case 0x000005B0 of processingStandardMessages.c because:
		 // the system frame 0x5B0 containing the button status is repeated very slowly (1-2sec interval),
		 // PDC system reacts on Release D1:0x00 after Push D1:0x20
		 // so the speed of react would be random and depend on the moment we sent the disable between the system frames
		 // that's why we follow push with our independent release code
		 //=========================================================================

		if (pdcAutoDisableEnabled){
		    if (requestToTogglePDC == 1) {
		        if (pdc_send_counter == 0) {
		            pdc_send_counter = 1;
		            last_pdc_shot_time = currentTime;
		            pdcMsgData[1] = 0x20;  // push button
		            can_tx(&pdcMsgHeader, pdcMsgData); // sent
		        }

		        // Prepare to send pdc button release code
		        if (pdc_send_counter == 1 && (currentTime - last_pdc_shot_time > 50)) { //changing the time we can allow for short beep before PDC disabling
		            pdc_send_counter = 0;
		            pdcMsgData[1] = 0x00;  // release button
		            can_tx(&pdcMsgHeader, pdcMsgData); // sent
		            requestToTogglePDC = 0; // set after push and release done
		        }
		    }
		}

//		=========================================================================
//		Simplified version with no release simulation
//		=========================================================================
//		if (pdcAutoDisableEnabled){
//			if (requestToTogglePDC == 1) {
//				requestToTogglePDC = 0;
//				pdcMsgData[1] = 0x20;  // push button// push button sent
//				can_tx(&pdcMsgHeader, pdcMsgData);
//			}
//		}
		// @netzmark PDC DISABLE code - end
*/
	}


	void dynoToggle(){
		if(DynoStateMachine == 0xff){ // there is no dyno Start sequence in progress
			DynoStateMachine=0; //state machine
			ESCandTCinversion=0; //do not change ESC and TC if dynomode is requested
			DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1; //length of DIAGNOSTIC SESSION msg
			can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
			onboardLed_blue_on();
			DynoStateMachineLastUpdateTime=currentTime;//save last time seen
			//wait the feedback from ECU
		}
	}

#endif
