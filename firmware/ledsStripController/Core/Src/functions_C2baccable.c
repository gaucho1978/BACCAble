/*
 * functions_C2baccable.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */
#include "functions_C2baccable.h"

#if defined(C2baccable)
	static void pdcMuteProcess(void); //park sensors mute 28/08/2026 - defined below C2PeriodicCheck(), which calls it

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
		pdcMuteProcess(); //park sensors mute 28/08/2026
	}

	//park sensors mute 28/08/2026 - BEGIN
	//Silences the front obstacle chime while standing still by pressing the park sensors button for the driver
	//(message 0x5B0), instead of rewriting the alarm message 0x3E7 the car is sending.
	//
	//The button is a toggle, so the whole point of what follows is that NOTHING here keeps its own idea of
	//whether the sensors are on or off. parkSensorsFunctionStatus, decoded from 0x54A, is the truth; a press is
	//sent only when it disagrees with what we want. A press that goes lost leaves the two still disagreeing and
	//is simply sent again, and a press we did not make (the driver's own, or the car switching the sensors back
	//on by itself above a certain speed) is seen for what it is at the next evaluation. There is no counter that
	//can drift, which is what would otherwise make the function work backwards after a few stops and starts.
	static void pdcMuteProcess(void){
		//nothing to do, and nothing left behind to undo
		if(parkSensorsMuteFunctionEnabled==0 && pdcMutedByUs==0 && pdcPressPhase==0) return;

		//A press in progress: release the button once it has been held long enough. Nothing else is decided
		//while we are here, so a second press can never be started on top of the first one.
		if(pdcPressPhase==1){
			if(currentTime-pdcPressTime>=TIMING__C2____PDC_BUTTON_PRESS_MS){
				pdcMsgData[1]=0x00; //button released
				can_tx(&pdcMsgHeader, pdcMsgData);
				pdcPressPhase=0;
				pdcLastPressTime=currentTime;
			}
			return;
		}

		if(pdcStateKnown==0) return; //0x54A never arrived: we do not know what the sensors are doing
		//Leaves the car time to answer on 0x54A before anything is judged again, and keeps a car creeping in a
		//queue from toggling the sensors over and over. It never delays the first press.
		if(currentTime-pdcLastPressTime<TIMING__C2____PDC_MIN_INTERVAL_BETWEEN_PRESSES_MS) return;

		//carSteadyCounter>1 is about 20msec of standing still (0x116 arrives every 10msec): the mute has to be
		//quick, and it is the comparison below - not this threshold - that keeps the logic straight.
		uint8_t wantSensorsOff = parkSensorsMuteFunctionEnabled && (carSteadyCounter>1) && (reverseGearActive==0);
		uint8_t sensorsAreOff  = (parkSensorsFunctionStatus==0); //0=off, the state the button leaves them in

		if(wantSensorsOff==sensorsAreOff){ //already as wanted
			pdcPressAttempts=0;
			//the sensors are on again and it is not because we asked: the driver pressed the button, or the car
			//switched them back on by itself. Either way it is no longer our doing.
			if(sensorsAreOff==0) pdcMutedByUs=0;
			return;
		}

		//The park sensors are not reacting: stop insisting, but ONLY when what we are asking for is to switch them
		//off. Giving up on switching them back on would leave them off while the car is moving, which is the one
		//outcome this function must never produce - so that direction is retried for as long as it takes.
		if(wantSensorsOff && pdcPressAttempts>=PDC_MAX_PRESS_ATTEMPTS) return;

		//We only ever switch them off on our own initiative, and we only ever switch them back on if it was us
		//who switched them off: a choice made by the driver is never undone here.
		if(wantSensorsOff==0 && pdcMutedByUs==0) return;

		pdcMsgData[1]=0x20; //button pressed (byte 1, bit 5)
		can_tx(&pdcMsgHeader, pdcMsgData);
		pdcPressPhase=1;
		pdcPressTime=currentTime;
		pdcPressAttempts++;
		//Deliberately only ever SET here, never cleared: it is cleared above, once 0x54A confirms the sensors are
		//really on again. Clearing it at this point would throw away the very flag that allows a failed switch on
		//to be tried again, and the sensors would be left off.
		if(wantSensorsOff) pdcMutedByUs=1;
	}
	//park sensors mute 28/08/2026 - END


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
