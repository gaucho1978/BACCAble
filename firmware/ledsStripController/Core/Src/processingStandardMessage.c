/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingStandardMessage.h"

void processingStandardMessage(){
	#if defined(C1baccable)
		if(function_route_msg_enabled==1){
			if(routeStdIdMsg==1){ //if we have to do it (std msg id route request


				if(rx_msg_header.StdId==routeMsgId){ //received msg to route
					routeStdIdMsg=0xFF; //set this to disable the request. only one message is routed to avoid bus flood
					if(routeOffset<rx_msg_header.DLC){ //send only if offset is correct
						uint8_t sizeToCopy=5; //
						if((rx_msg_header.DLC - routeOffset )<sizeToCopy) sizeToCopy=rx_msg_header.DLC - routeOffset;
						memcpy(&routeMsgData[3],&rx_msg_data[routeOffset],sizeToCopy);
						if(sizeToCopy<5) memset(&routeMsgData[3+sizeToCopy],0x00, 5-sizeToCopy);

						//send it
						can_tx(&routeMsgHeader, routeMsgData);
						onboardLed_blue_on();
					}
				}
			}
		}
	#endif

	switch(rx_msg_header.StdId){ //messages in this switch is on C can bus, only when on different bus, the comments explicitly tells if it is on another can bus

		case 0x00000090:
			#if defined(BHbaccable) //the car is showing something (ie:radio name) on the dashboard
				//override the displaied string, by restarting the frame
				paramsStringCharIndex=0; //prepare to send first char of the string
				telematic_display_info_field_frameNumber=0; //prepare to send first frame
				if((requestToSendOneFrame==0) && (dashboardPageStringArray[0]!=' ')) requestToSendOneFrame++; //if message is not empty (we check only first char) and if required, increment messages sequence to send, in order to send at least one
				lastSentTelematic_display_info_msg_Time=0; //if required will immediately send a sequence
			#endif
			//on BH can bus, slow bus at 125kbps, this message contains:

			//total frame number is on byte 0 from bit 7 to 3
			//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
			//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
			//UTF text 1 is on byte 2 and byte 3
			//UTF text 2 is on byte 4 and byte 5
			//UTF text 3 is on byte 6 and byte 7
			break;
		case 0x000000FB:

			#if defined(C1baccable)
				if(rx_msg_header.DLC>=4){
					torque= ((rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111));
				}
			#endif
			//torque is on byte 2 from bit 6 to 0 and byte 3 from bit 7 to 4.
			break;

		case 0x000000FC:
			processingMessage0x000000FC();
			break;
		case 0x00000101:
			#if defined(C1baccable)
				if(rx_msg_header.DLC>=3){
					//get vehicle speed
					currentSpeed_km_h= (float)((((uint16_t)rx_msg_data[0] << 11) & 0b0001111111111111) | ((uint16_t)rx_msg_data[1] <<3) | ((uint16_t)rx_msg_data[2] >>5))/16;
				}
			#endif
			break;
		case 0x000001EF:
			processingMessage0x000001EF();
			break;
		case 0x000001F0:
			//clutch interlock is on byte 0 bit 7
			//clutch upstop is on byte0 bit 6
			//actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
			//analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.

			break;
		case 0x0000001F7:
			#if defined(C1baccable)
				if(rx_msg_header.DLC>=4){
					transmissionTemperature= ((rx_msg_data[2] & 0b00000001) << 5 | ((rx_msg_data[3] >> 3) & 0b00011111));
					//onboardLed_blue_on();
				}
			#endif
			//transmission temperature is on byte 2 bit 0 and byte 3 from bit 7 to bit 3
			break;
		case 0x000001FC: //received on C2 can bus
			//Rear Diff. Warning La. is on byte0 bit7
			//Rear Diff, Control Status is on byte0 bit6
			//Active Dumping Control Status (the suspensions) is on byte0 from bit 5 to 4 (0x0=Mid, 0x1=Soft, 0x2=Firm [only on QV])
			//Rd. Asp. Ind. is on byte 0 from bit3 to 0 and byte 1 from bit 7 to 4
			//Active Dumping Control Fail status is on byte 1 bit3
			//Aero. Fail Status is on byte 1 bit2
			//Front Aero. status is on byte 1 bit1 to bit0
			//CDCM warning lamp is on byte 2 bit5
			break;
		case 0x00000226:
			processingMessage0x00000226();
			break;
		case 0x000002ED: //message to dashboard containing shift indicator
			processingMessage0x000002ED();
			break;
		case 0x000002EE: // presente solo su BH can bus a 125kbps
			//this message contains the following radio buttons on the steering wheel:
			//radio right button is on byte 3 bit6 (1=button pressed)
			//radio left button on the steering wheel is on byte 3 bit4 (1=button pressed)
			//radio Voice command button is on byte 3 bit2 (1= button pressed)
			//phone call button is on byte3 bit0(1=button pressed)
			//volume  is on byte 4 (volume up increases the value, volume down reduces the value. once arrived to 255 restarts from 0 and under 0 goes to 255)
			//volume change is on byte5 bit 7 and bit6 (1=volume was increased rotation, 2=volume decreased rotation, 3=volume mute button press) (then reading the entire byte we will see respectively, 0x40, 0x80,  0xC0)
			//sample: uint8_t tmpCmd=rx_msg_data[5] >>6; //1=volume was increased rotation, 2=volume decreased rotation
			break;
		case 0x000002EF: //se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
			#if defined(C1baccable)
				currentGear=rx_msg_data[0] & ~0xF;

				if(function_led_strip_controller_enabled==1){
					scaledColorSet=scaleColorSet(currentGear); //prima di tutto azzeriamo i primi 4 bit meno significativi, poi scala il dato con la funzione scaleColorSet, per prepararlo per l'invio alla classe vumeter
					vuMeterUpdate(scaledVolume,scaledColorSet);
				}

				//if(function_regeneration_alert_enabled){
				//	if((rx_msg_data[1] >>7)==1 && regenerationInProgress==0){ //if regeneration has just begun,
				//		uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
				//		addToUARTSendQueue(tmpArr3, 1);
				//	}
				//}
				//regenerationInProgress=rx_msg_data[1] >>7; //DPF Regeneration mode is on byte 1 bit 7.

			#endif

			//actual gear status is on byte 0 from bit 7 to 4 (0x0=neutral, 0x1 to 0x6=gear 1 to 6, 0x07=reverse gear, 0x8 to 0xA=gear 7 to 9, 0xF=SNA)
			//suggested gear status is on byte 0 from bit 3 to 0
			//DPF Regeneration mode is on byte 1 bit 7.
			//SAM info is on byte 1 from bit 3 to 0
			//stop start fault status is on byte 2 bit 7
			//..
			//boost pressure indication is on byte 3 bit from 6 to 0 and byte 4  bit 7


			break;
		case 0x000002FA:
			processingMessage0x000002FA();
			break;
		case 0x00000384:
			processingMessage0x00000384();
			break;
		case 0x00000412: //se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il valore é >51 (sfrutto le info ottenute sniffando)
			#if defined(C1baccable)
				if(function_led_strip_controller_enabled==1){
					if( (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51) ){
						timeSinceLastReceivedAcceleratorMessage=currentTime;
						ledsStripIsOn=1;
						scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
						vuMeterUpdate(scaledVolume,scaledColorSet);
					}
				}
			#endif
			break;
		case 0x0000041A:
			#if defined(C1baccable)
				if(rx_msg_header.DLC>=6){
					batteryStateOfCharge= (rx_msg_data[1] & 0b01111111); //set Most Significant Bit to zero
					batteryCurrent= (rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111));
				}
			#endif
			//battery state of charge is on byte 1 from bit 6 to 0 (Percentage)
			//battery current (A) is on byte 4 and in byte 5 from bit 7 to bit 4
			break;
		case 0x000004B1:
			#if defined(C1baccable)
				if(requestToDisableStartAndStop==1){//if requested, send message to simulate button press
					requestToDisableStartAndStop=0;
					startAndStopEnabled=0; //done
					memcpy(&disableStartAndStopMsgData, &rx_msg_data, rx_msg_header.DLC); //grab the message
					disableStartAndStopMsgHeader.DLC=rx_msg_header.DLC;
					//set message data, byte 5, bits from 5 to 3 to binary 001.
					disableStartAndStopMsgData[5]=(disableStartAndStopMsgData[5] & 0b11000111) | (0x01<<3);
					can_tx(&disableStartAndStopMsgHeader, disableStartAndStopMsgData);
					onboardLed_blue_on();

				}
			#endif
			// Bonnet Status is on byte0 bit 4
			// driver door Fail status is on byte0 bit 3
			// FOB Search Request is on byte 0 bit from 2 to 1
			// Driver door status is on byte 0 bit 0
			//Passenger Door status is on byte1, bit 7
			//Left  Rear Door status is on byte 1 bit 6
			//Right Rear Door status is on byte 1 bit 5
			//Rear Hatch Status is on byte 1 bit 4
			//Rear Heated Window Status is on byte 1 bit 3
			//Front Heated Window Status is on byte 1 bit 2
			//Theft Alarm Status is on byte 2 from bit 6 to 4
			//Remote start Inhibit Status is on byte 2 from bit 3 to 0 and byte 3 from bit 7 to 6
			//Remote start Active status is on byte 3 bit 5
			//Battery state of function is on byte 3 from bit 4 to 0 and byte 4 bit 7
			//compressor Air Conditioner status is on byte 4 bit 5
			//Recalibration is on byte 4 bit 3
			//Exterior Rear Release Switch Status is on byte 4 bit 1
			//Start&Stop Pad1 is on byte 5 from bit 5 to 3 (value 1 enables and disables Start& stop)

			break;
		case 0x000004B2:
			#if defined(C1baccable)
				if(rx_msg_header.DLC>=4){
					oilPressure= ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111));
					oilTemperature= ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011));
				}
			#endif
			//engine oil level is in byte 0 from bit 7 to 3.
			//engine oil over fill status is on byte 0, bit 2.
			//engine oil min. is on byte 0 bit 1
			//engine oil pressure is on byte 0, bit 0 and on byte 1 from bit 7 to 1. (bar)
			//power mode status is on byte 1 bit 0 and on byte 2 bit 7.
			//engine water level is on byte 2 bit 6.
			//engine oil temperature is on byte 2 from bit 5 to 0 and on byte 3 from bit 7 to 6.
			//engine oil temperature warning light is on byte 3 bit 5.
			break;
		case 0x00000545:
			#if defined(C1baccable)
				if(rx_msg_header.DLC==8){
					memcpy(&dashboardBlinkMsgData, &rx_msg_data, 8);
				}
			#endif
			//only if  lights are ON, and therefore the dashboard is  set to max brightness: setting byte 5 to 0x00, the brightness increases for around 100msec (this works for any value between 0x and 7x )
			//only if lights are OFF, and therefore the dashboard is set to min brightness: setting byte 5 to 0xF0, the brightness reduces for around 100msec (this works for any value between Dx and Fx)
			//this is the test message to increase brightness: 0x88 0x20 0xC3 0x24 0x00 0x14 0x30 0x00
			break;
		case 0x000005A5:
			//cruise control ON/OFF status is on byte0 bit7 (0=disabled, 1=enabled)
			#if defined(C1baccable)
				if((rx_msg_data[0]>>7)==1){
					cruiseControlDisabled=0;//disable additional parameter menu commands
					//onboardLed_blue_on();
				}else{
					cruiseControlDisabled=1;//enable additional parameter menu commands
					//onboardLed_red_on();
				}
			#endif
			break;
		case 0x000005AC:
			#if defined(BHbaccable)
				if(requestToPlayChime==1){  //if there is a request to play sound
					requestToPlayChime=0;
					//copy message
					memcpy(CHIME_msg_data, &rx_msg_data, rx_msg_header.DLC);
					//enable chime, by changing message
					CHIME_msg_header.DLC=rx_msg_header.DLC;
					CHIME_msg_data[0]= (CHIME_msg_data[0] & 0b00111111) ; //set bit 7 and 6 to zero (chime type 0)
					CHIME_msg_data[1]= (CHIME_msg_data[1] & 0b00111111) | 0b01000000; //byte1 bit 7 and 6 = 01 (seatbelt alarm active)
					CHIME_msg_data[3]=CHIME_msg_data[3] | 0xE0; //max volume
					can_tx(&CHIME_msg_header, CHIME_msg_data); //send msg
					onboardLed_blue_on();
				}
			#endif
			break;
		case 0x000005AE:
			#if defined(C1baccable)
				//this message is directed to IPC once per second. DPF status is on byte 4 bit 2. (1=dirty, 0=clean)
				if(rx_msg_header.DLC>=6){
					dieselEngineRegenerationMode = (rx_msg_data[5]>>2 ) & 0b00000111 ;//byte 5 bit 4-2

					if(function_regeneration_alert_enabled){  //if  function was enabled in setup menu
						if(regenerationInProgress){ //if regeneration is in progress
							if(((rx_msg_data[4]>>2) & 0x01 )==0){ //if the message needs to be changed
								#ifdef DPF_REGEN_VISUAL_ALERT
									//change message and send it again
									memcpy(STATUS_ECM_msg_data, &rx_msg_data, rx_msg_header.DLC); //copy message
									STATUS_ECM_msg_data[4] |= 0x04; //DPF Dirty (bit 2) set to ON
									STATUS_ECM_msg_header.DLC=rx_msg_header.DLC;
									can_tx(&STATUS_ECM_msg_header, STATUS_ECM_msg_data); //send msg
								#endif
								onboardLed_blue_on();
							}

						}

						if((dieselEngineRegenerationMode==2) && (regenerationInProgress==0)){ //if regeneration has just begun,
							#ifdef DPF_REGEN_SOUND_ALERT
								uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
								addToUARTSendQueue(tmpArr3, 1);
							#endif
							regenerationInProgress=1;
						}
						if((dieselEngineRegenerationMode==0) && (regenerationInProgress==1)){ //if regeneration has ended,
							regenerationInProgress=0;
						}
					}

				}
			#endif
			break;
		case 0x000005B0:
			#if defined(C2baccable)
				if((rx_msg_data[1] == 0x20) && ( DynoStateMachine == 0xff)){ // park assist button was pressed and there is no dyno Start sequence in progress
					ParkAssistButtonPressCount++;
					if (ParkAssistButtonPressCount>5){ // more or less 6 seconds
						ParkAssistButtonPressCount=0; //reset the count
						dynoToggle();
						//wait the feedback from ECU
					}
				}else{
					ParkAssistButtonPressCount=0;// reset the count assigning it zero
				}
			#endif
			//the park assistant button press event is on byte 1 bit 5 (1=pressed)
			break;
		case 0x0000073A:
			//contains current date from byte 0 to 7.
			//Hex values are used as characters in example 0x21 0x02 0x26 0x01 0x20 0x 25 represents
			//the date h21 minutes 02 day 26 month 01 year 2025.
			//last two bytes of the message are 00 00.
			break;
		case 0x0000073C:
			#if defined(C1baccable)
				if(rx_msg_header.DLC>=8){
					switch ((rx_msg_data[7]>>4) & 0x07) {
						case 0x00: //ACC is off
							//onboardLed_red_on();
							ACC_Disabled=1; //enable additional parameter menu commands
							ACC_engaged=0; //acc not engaged
							break;
						case 0x02: //acc engaged
							ACC_engaged=1;
							ACC_Disabled=0; //disable additional parameter menu commands
							break;
						default:
							ACC_Disabled=0; //disable additional parameter menu commands
							ACC_engaged=0; //acc not engaged
					}
				}
			#endif
			//contains status of ACC on byte 7, from bit 6 to 4 (0=disabled, 1=enabled, 2=engaged 3=engaged brake only, 4=override, 5=cancel)
			break;
		default:
	}


}
