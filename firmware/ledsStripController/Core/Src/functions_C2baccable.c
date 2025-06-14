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
			}
		}

		if(front_brake_forced==255){ //request to disable Front brake
			front_brake_forced=0;
			//just reply to C1 baccable
			uint8_t tmpArr[2]={C1BusID,C2cmdNormalFrontBrake};
			addToUARTSendQueue(tmpArr, 2);
			//can_tx(&rearBrakeMsgHeader[0], rearBrakeMsgData[0]); //send message to return control to ECU


		}

		if(front_brake_forced==5){
			front_brake_forced=4;
			//send reply via serial line to C1 to inform that front brake is going to be forced
			uint8_t tmpArr[2]={C1BusID,C2cmdForceFrontBrake};
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
