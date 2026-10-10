/*
 * functions_C2baccable.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */
#include "functions_C2baccable.h"

#if defined(C2baccable)

	//park mute brake from C1 10/10/2026 - the park mute decision, moved here unchanged from case 0x107 of
	//processingStandardMessage.c: 0x107 (brake) is not on the C2 bus, so it never ran. The brake state now comes from C1
	//over uart (parkMuteBrakePressed); the other inputs are still read here on C2 (0xFC reverse, 0x3E7 beeping, 0x54A
	//led). It is evaluated every 30 ms (0x107 used to trigger it every 10 ms), and only with the function enabled
	//(checked by the caller, C2PeriodicCheck).
	static void parkMuteEvaluate(void){
		//sniffer tx/debug 27/09/2026 - park mute: snapshot of every input of the decision below, sent only when one of
		//them changes (evaluated every 30 ms, sending it every time would just flood the sniff). One byte per value:
		//  value 1 = [byte0] function enabled (always 1 now: evaluated only with the function on), [byte1] brake pressed (parkMuteBrakePressed, from C1),
		//            [byte2] reverseGearActive, [byte3] pdc_is_beeping
		//  value 2 = [byte0] pdc_auto_disabled, [byte1] parkSensorsLedStatus, [byte2] requestToTogglePDC,
		//            [byte3] always 0 (was the raw brake byte of 0x107, not available on C2) //park mute brake from C1 10/10/2026
		if(snifferInUse){
			static uint32_t snifferParkMuteLastState1=0xFFFFFFFF;
			static uint32_t snifferParkMuteLastState2=0xFFFFFFFF;
			uint32_t snifferParkMuteState1=	((uint32_t)parkSensorsMuteFunctionEnabled)				|
											((uint32_t)parkMuteBrakePressed<<8)				| //brake pressed, from C1 (raw*0.4>14.5 on 0x107)
											((uint32_t)reverseGearActive<<16)					|
											((uint32_t)pdc_is_beeping<<24);
			uint32_t snifferParkMuteState2=	((uint32_t)pdc_auto_disabled)						|
											((uint32_t)parkSensorsLedStatus<<8)				|
											((uint32_t)requestToTogglePDC<<16);
			if(snifferParkMuteState1!=snifferParkMuteLastState1 || snifferParkMuteState2!=snifferParkMuteLastState2){
				snifferParkMuteLastState1=snifferParkMuteState1;
				snifferParkMuteLastState2=snifferParkMuteState2;
				SNIFFER_DEBUG2(0x2100, snifferParkMuteState1, snifferParkMuteState2); //park mute: inputs changed
			}
		}
		//reverse gear: the car is *supposed* to enable the PDC by itself, but it doesn't always -
		//check the actual LED status and re-enable ourselves if the sensors are still really off.
		if (reverseGearActive == 1) {
			if(pdc_auto_disabled == 1){
				if (parkSensorsLedStatus == 1  ) { //led continuous -> park sensors still really disabled
					requestToTogglePDC = 1;
				}
				SNIFFER_DEBUG2(0x2101, parkSensorsLedStatus, requestToTogglePDC); //park mute: reverse gear, our marker dropped (value 2 = 1: sensors re-enabled by us) //sniffer tx/debug 27/09/2026
				pdc_auto_disabled = 0;
			}
		}else{ //reverse gear not engaged
			//DISABLE: brake pressed firmly enough and the sensors are beeping. Not while in reverse.
			if (parkMuteBrakePressed){ //brake pressed firmly (C1 applies the 14.5% threshold on 0x107) //park mute brake from C1 10/10/2026
				if((pdc_is_beeping == 1) && (pdc_auto_disabled == 0)) {
					if (parkSensorsLedStatus != 1) { //led not continuous -> park sensors currently on
						requestToTogglePDC = 1;
						pdc_auto_disabled = 1;
						SNIFFER_DEBUG1(0x2102, parkSensorsLedStatus); //park mute: brake pressed while beeping -> disable the sensors //sniffer tx/debug 27/09/2026
					}
				}
			}else{ //brake released and the car is able to move again

				if(0){//if we are in P, just disable PDC (now we don't know if we are in P (seems to be not available on C2 bus, so just temporary exclude this part of the code, while testing it (we need to be sure if we need this logic).
					if((pdc_is_beeping == 1) && (pdc_auto_disabled == 0)) {
						if (parkSensorsLedStatus != 1) { //led not continuous -> park sensors currently on
							requestToTogglePDC = 1;
							pdc_auto_disabled = 1;
						}
					}
				}else{ //we are not in rear drive neither in P, and brake is released, so maybe we're moving forward
					if(pdc_auto_disabled == 1 && parkSensorsLedStatus == 1) { //we switched them off and they really are
						requestToTogglePDC = 1; //re enable sensors since we are moving forward probably
						pdc_auto_disabled = 0;
						SNIFFER_DEBUG(0x2103); //park mute: brake released -> re-enable the sensors //sniffer tx/debug 27/09/2026
					}else if (pdc_auto_disabled == 1 && parkSensorsLedStatus != 1) { //the car put them back on by itself (speed exceeded): just drop the marker
						pdc_auto_disabled = 0; //may be car re enabled sensors by itself, just forget internal status.
						SNIFFER_DEBUG1(0x2104, parkSensorsLedStatus); //park mute: brake released, the car had already re-enabled them //sniffer tx/debug 27/09/2026
					}
				}
			}
		}


	}

	//readFaults ABS 10/10/2026 - BEGIN
	//The ABS faults are read here, on the C2 bus, for Read Faults on C1 (protocol in uart.h, FAULTS ABS PROTOCOL). C1 asks the
	//reading with C2cmdAbsFaultsStart and collects the result with C2cmdAbsFaultsGet; C2 answers only inside its normal reply
	//window, with messages whose payload is made of letters and hex digits only. The decoding of the ABS answers is in
	//processingExtendedMessage.c. The elm327 bridge is never used.

	//1 while the ABS is used by dyno or front brake (same ECU 0x18DA28F1): the reading is not started, or is stopped
	static uint8_t absFaultsAbsBusy(void){
		return (DynoStateMachine!=0xff || DynoModeEnabled || front_brake_forced!=0) ? 1 : 0;
	}

	//queues up to ABS_FAULTS_REPLY_MSGS messages for C1, from the DTC of index "index" (2 DTC per message)
	static void absFaultsSendReplies(uint8_t index){
		static const char hx[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
		uint8_t m[UART_BUFFER_SIZE];
		for(uint8_t k=0; k<ABS_FAULTS_REPLY_MSGS; k++){
			uint8_t first=(uint8_t)(index+2*k);
			if(k>0 && (absFaultsState!=3 || first>=absFaultsDTCcount)) break; //only the list continues over more messages
			memset(m, ' ', UART_BUFFER_SIZE);
			m[0]=C1BusID;
			m[1]=C1cmdAbsFaultsReply;
			if(absFaultsState==3){
				uint8_t total=(absFaultsDTCtotal>0xFF) ? 0xFF : (uint8_t)absFaultsDTCtotal;
				m[2]='R';
				m[3]=hx[total>>4];
				m[4]=hx[total&0xF];
				m[5]=(uint8_t)('A'+absFaultsDTCcount);
				m[6]=(uint8_t)('A'+first);
				for(uint8_t j=0; j<2; j++){
					uint8_t i=(uint8_t)(first+j);
					if(i>=absFaultsDTCcount) break;
					for(uint8_t b=0; b<3; b++){
						m[7+6*j+2*b]=hx[absFaultsDTCbytes[i][b]>>4];
						m[8+6*j+2*b]=hx[absFaultsDTCbytes[i][b]&0xF];
					}
				}
			}else if(absFaultsState<3){
				m[2]='B'; //reading in progress
			}else{
				m[2]='E'; //failed, or never started (C2 restarted in the meantime)
			}
			addToUARTSendQueue(m, UART_BUFFER_SIZE);
		}
	}

	static void absFaultsProcess(void){
		if(absFaultsStartRequest){
			absFaultsStartRequest = 0;
			absFaultsDTCcount   = 0;
			absFaultsDTCtotal   = 0;
			absFaultsRxReceived = 0;
			absFaultsRxExpected = 0;
			absFaultsRecordFill = 0;
			absFaultsRxNextSN   = 1;
			absFaultsResponsePending = 0;
			absFaultsTimer      = currentTime;
			if(absFaultsAbsBusy()){
				absFaultsState = 4; //ABS busy with dyno / front brake: C1 shows the faults of BODY and ECM
			}else{
				absFaultsTxHeader.DLC = 3;
				absFaultsTxData[0]    = 0x02; // PCI: SF 2 byte
				absFaultsTxData[1]    = 0x10; // SID: DiagnosticSessionControl
				absFaultsTxData[2]    = 0x03; // sub: extendedDiagnosticSession
				can_tx(&absFaultsTxHeader, absFaultsTxData);
				absFaultsState = 0;
			}
		}
		if(absFaultsState<3){
			if(absFaultsAbsBusy()){
				absFaultsState = 4; //dyno or front brake started meanwhile: they own the ABS
			}else if(currentTime-absFaultsTimer > (absFaultsResponsePending ? 5000 : 2000)){
				absFaultsState = 4; //no answer from the ABS
			}
		}
		if(absFaultsState!=snifferAbsFaultsLastState){
			SNIFFER_DEBUG2(0x2300, ((uint32_t)snifferAbsFaultsLastState<<8)|absFaultsState, ((uint32_t)absFaultsDTCtotal<<16)|absFaultsDTCcount); //ABS faults: state changed. v1=byte1 old, byte0 new (0xFF never started, 0 session, 1 ReadDTC, 2 multiframe, 3 ready, 4 failed), v2=high 16 bit valid DTC in total, low 16 bit DTC kept
			snifferAbsFaultsLastState=absFaultsState;
		}
		if(absFaultsGetIndex!=0xFF){
			uint8_t index=absFaultsGetIndex;
			absFaultsGetIndex=0xFF;
			absFaultsSendReplies(index);
		}
	}
	//readFaults ABS 10/10/2026 - END

	void C2PeriodicCheck(){
		//dyno debug 04/10/2026 - dynoToggle() may run inside the uart rx interrupt (C2cmdtoggleDyno), where tracing
		//is not allowed: dyno status changes are therefore detected here, in the main loop, by comparing with the last traced values
		if(DynoModeEnabled!=snifferDynoLastModeEnabled || DynoStateMachine!=snifferDynoLastStateMachine){
			SNIFFER_DEBUG2(0x2200, ((uint32_t)snifferDynoLastModeEnabled<<8)|snifferDynoLastStateMachine, ((uint32_t)DynoModeEnabled<<8)|DynoStateMachine); //dyno: status changed. v1=old, v2=new (byte1=DynoModeEnabled, byte0=DynoStateMachine) //dyno debug 04/10/2026
			snifferDynoLastModeEnabled=DynoModeEnabled;
			snifferDynoLastStateMachine=DynoStateMachine;
		}

		if(DynoStateMachine!=0xff){ //if state machine in progress
			if(currentTime-DynoStateMachineLastUpdateTime> 4000){ //if older than 4 sec
				SNIFFER_DEBUG2(0x2204, DynoStateMachine, DynoModeEnabled); //dyno: state machine timeout, no reply from ABS within 4 sec //dyno debug 04/10/2026
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
		absFaultsProcess(); //ABS faults read for Read Faults on C1 //readFaults ABS 10/10/2026

		//park mute brake from C1 10/10/2026 - park mute decision, every 30 ms, only with the function enabled (see parkMuteEvaluate)
		if(parkSensorsMuteFunctionEnabled){
			if(currentTime-parkMuteLastEvaluationTime>=30){
				parkMuteLastEvaluationTime=currentTime;
				parkMuteEvaluate();
			}
		}

		// @netzmark PDC DISABLE code - simulated push of the park sensors button, followed by our own release.
		// Not done in case 0x5B0 because that frame is repeated every 1-2sec and the PDC reacts on the release.
		if (parkSensorsMuteFunctionEnabled){
		    if (requestToTogglePDC == 1) {
		        if (pdc_send_counter == 0) {
		            pdc_send_counter = 1;
		            last_pdc_shot_time = currentTime;
		            pdcMsgData[1] = 0x20;  // push button
		            can_tx(&pdcMsgHeader, pdcMsgData);
		            SNIFFER_DEBUG1(0x2110, pdc_auto_disabled); //park mute: button push queued //sniffer tx/debug 27/09/2026
		        }

		        // raising the hold time allows a short beep before the PDC is disabled
		        if (pdc_send_counter == 1 && (currentTime - last_pdc_shot_time > TIMING__C2____PDC_BUTTON_PRESS_MS)) {
		            pdc_send_counter = 0;
		            pdcMsgData[1] = 0x00;  // release button
				can_tx(&pdcMsgHeader, pdcMsgData);
		            SNIFFER_DEBUG1(0x2111, pdc_auto_disabled); //park mute: button release queued //sniffer tx/debug 27/09/2026
		            requestToTogglePDC = 0; // cleared once push and release are both done
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
