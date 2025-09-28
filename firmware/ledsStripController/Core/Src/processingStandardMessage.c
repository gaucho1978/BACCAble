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
				if(ESCandTCinversion){ //if esc/tc is active, don't show baccable menu
					 requestToSendOneFrame=0;
				}else{
					//override the displaied string, by restarting the frame
					paramsStringCharIndex=0; //prepare to send first char of the string
					telematic_display_info_field_frameNumber=0; //prepare to send first frame
					if((requestToSendOneFrame==0) && (dashboardPageStringArray[0]!=' ')) requestToSendOneFrame++; //if message is not empty (we check only first char) and if required, increment messages sequence to send, in order to send at least one
					lastSentTelematic_display_info_msg_Time=0; //if required will immediately send a sequence
				}
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
					torque=(torque-500);
					#ifdef TORQUE_CORRECTION_FACTOR
						torque=torque * TORQUE_CORRECTION_FACTOR; //custom correction factor in user_define.h
					#endif
				}

				if(launch_assist_enabled==1){
					if(torque>=launch_torque_threshold){
						//send serial message to C2 baccable, to RELEASE front brakes
						uint8_t tmpArr3[2]={C2BusID,C2cmdNormalFrontBrake};
						addToUARTSendQueue(tmpArr3, 2);
						launch_assist_enabled=0; //ensure we do not return here
						//jump to statistics
						dashboard_menu_indent_level=1;
						main_dashboardPageIndex=1; 	// params submenu

						dashboardPageIndex=getParamIndexFromReqId(0x1A); 		// 0-100km/h statistics
					}
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

					//Execute Statistics
					if((previousSpeed_km_h<0.0625) && (currentSpeed_km_h>=0.0625)){ //we started. Let's count the time
						statistics_0_100_StartTime=currentTime-10;
						statistics_0_100_started = 1;
					}



					if(statistics_0_100_started){
						chronometerElapsedTime_0_100_km_h=((float)(currentTime-statistics_0_100_StartTime))/1000.0;

						if(currentSpeed_km_h>=100.0){ // target speed reached
							statistics_0_100_started=0;// stop the time
							saveStatisticsOnFlash();//save on flash, if it is a best time
						}

						if(chronometerElapsedTime_0_100_km_h>20.0){ //if it is missed
							statistics_0_100_started=0;
						}
					}

					if((previousSpeed_km_h<=100.0) && (currentSpeed_km_h>100.0)){ //we started. Let's count the time
						statistics_100_200_StartTime=currentTime-10;
						statistics_100_200_started = 1;
					}

					if(statistics_100_200_started){
						chronometerElapsedTime_100_200_km_h=((float)(currentTime-statistics_100_200_StartTime))/1000.0;

						if(currentSpeed_km_h>=200.0){ 		//target speed was reached
							statistics_100_200_started=0;	// stop the time
							saveStatisticsOnFlash();			//save on flash, if it is a best time
						}

						if(chronometerElapsedTime_100_200_km_h>40.0){ //if it is missed
							statistics_100_200_started=0;
						}
					}

					previousSpeed_km_h=currentSpeed_km_h;

				}

			#endif
			break;

		case 0x00000192:
			#if defined(C1baccable)
					//if release button was pressed twice, toggle QV exhaust valve
					if((rx_msg_data[0] & 0b00001100)==0x04){ //release button is pressed
						if(ReleasebuttonPressBeginTime==0){ //if button was not pressed, and now it is pressed
							ReleasebuttonPressBeginTime=currentTime;//save current time it was pressed (press begin)
							numberOfReleaseButtonClicks++;
							if (numberOfReleaseButtonClicks==1) ReleasebuttonFirstClickTime=currentTime;
						}

						if ((currentTime-ReleasebuttonPressBeginTime)>2000) { //if pressed since 2 seconds
							onboardLed_blue_on();
							ReleasebuttonPressBeginTime=0; //reset the timer, like if it was not pressed
							numberOfReleaseButtonClicks=0;
						}

					}else{
						ReleasebuttonPressBeginTime=0; //use this value to remember that button is not pressed
						if(currentTime-ReleasebuttonFirstClickTime>1000){ // if more than 1 second is passed since first button click
							numberOfReleaseButtonClicks=0; //reset also the counter of the number of consecutive clics :-)
						}
						if(numberOfReleaseButtonClicks>=2){ //if double click
							numberOfReleaseButtonClicks=0; //ensure we don't return here :-)
							onboardLed_blue_on();
							//execute action :-)
							if(QV_exhaust_flap_function_enabled){
								if(ForceQVexhaustValveOpened==0){
									ForceQVexhaustValveOpened=1; //start override sequence
								}else{
									ForceQVexhaustValveOpened=4; //return control to ecu
								}

							}
						}
					}
				// P button, located on the gear shift lever, is on byte 0, bit 0 and 1 (3=failure, 2=pressed, 1=not pressed,0=init)
				// gear shift requested position is on byte 0 da bit 7 a bit 4.
				// release button, located on the gear shift lever, is on byte 0, bit 3 and 2 (0=not pressed, 1=pressed)
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
		case 0x0000025A:
			#if defined(BHbaccable)
				if (ESCandTCinversion){
					if(currentDNAmode!=0x0C){ //if we are not in race
						if((rx_msg_data[0] & 0x7C)!= 0x04){   //drive style is on byte 0 from bit 2 to 6 (without bit shift is: 0x40=natural, 0x20=dynamic, 0x10=Allweather, 0x04=race)
							rx_msg_data[0] = (rx_msg_data[0] & ~0x7C) | 0x04;
							can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
							//onboardLed_blue_on();
						}
					}
				}
			#endif

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
		case 0x00000356:
		   #if defined(BHbaccable)
			if(disable_odometer_blink){
				if((rx_msg_data[4] & 0x04) ==0x04){ //if proxy align is required (SysEOLsts=1)
					rx_msg_data[4] &= (uint8_t)~0x04; //set to zero the SysEOLsts
					can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //retransmit the packet
				}
			}
		   #endif
		   break;
		case 0x00000358:
			//should contain volume position (number of ticks in byte2, and direction in byte 2, bit 6 and 5)
			break;
		case 0x00000384:
			processingMessage0x00000384();
			break;
		case 0x000003E8:
			#if defined(BHbaccable)
				//gearEngaged is on byte 3 bit 3 to 0
				//actualGearGSI is on byte 3 bit 7 to 4
				currentGear= rx_msg_data[3] & 0x0F;
			#endif
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
					batteryCurrent= (rx_msg_data[4] << 4 | (rx_msg_data[5] >> 4));
				}
			#endif
			//battery state of charge is on byte 1 from bit 6 to 0 (Percentage)
			//battery current (A) is on byte 4 and in byte 5 from bit 7 to bit 4
			break;
		case 0x0000046C:
			#if defined(BHbaccable)

				if(currentDNAmode!=(rx_msg_data[7] & 0x1F)){ //RDNA mode was changed, reset the ESCandTCinversion
					ESCandTCinversion=0;
				}
				//current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 7, from bit 0 to bit 4 (0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
				currentDNAmode=rx_msg_data[7] & 0x1F; //1F is the mask from bit 4 to 0
				if (ESCandTCinversion){
					if(currentDNAmode!=0x0C){ //if not in race
						rx_msg_data[7] = (rx_msg_data[7] & ~0x1F) | 0x0C;  //set Race mode (0x30) to show on IPC the race screen
						can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
						//onboardLed_blue_on();
					}

				}

				//turn indicators are on byte 6, bit 2 and 1
				turnIndicator= (rx_msg_data[6]>>1) & 0x03; //0= center, 1=right, 2=left
			#endif
			break;
		case 0x000004AF:
			#if defined(C1baccable) || defined(C2baccable)
				if (ESCandTCinversion){
					if(currentDNAmode!=0x30){ //if we're not in race
						if(((rx_msg_data[0] & 0x01)!=0x01) || ((rx_msg_data[1] & 0x04)!=0x04)){  //if not set as expected in race
							rx_msg_data[0] = rx_msg_data[0] | 0x01;  //set function bit
							rx_msg_data[1] = rx_msg_data[1] | 0x04;  //set function bit
							can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
						}
					}
				}
			#endif
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
		case 0x000004B4:
			#if defined(C2baccable)
			#endif

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
		case 0x000005A0:
			#if defined(BHbaccable)

				//Lane button press on some giulias (most recent?) is only on BH, byte4, bit 0. it is sent once per second and on variation

				if((rx_msg_data[4] & 0x01) ==0x01){ // left stalk button was pressed (lane following indicator)
					if(LANEbuttonPressBeginTime==0){ //if button was not pressed, and now it is pressed
						LANEbuttonPressBeginTime=currentTime;//save current time it was pressed (press begin)
						numberOfLaneButtonClicks++;
						if (numberOfLaneButtonClicks==1) LANEbuttonFirstClickTime=currentTime;
					}

					if ((currentTime-LANEbuttonPressBeginTime)>2000) { //if pressed since 2 seconds
						//notify to C2, that LANE was pressed for more than 3 seconds -> request to toggle ESC/TC
						if(function_esc_tc_customizator_enabled){
							uint8_t tmpArr1[2]={C1BusID, C1cmdLaneSingleTap};
							addToUARTSendQueue(tmpArr1, 2);
							onboardLed_blue_on();
						}
						LANEbuttonPressBeginTime=0; //reset the timer, like if it was not pressed
						numberOfLaneButtonClicks=0;
					}
				}else{
					LANEbuttonPressBeginTime=0; //use this value to remember that button is not pressed

					if(currentTime-LANEbuttonFirstClickTime>1000){ // if more than 1 second is passed since first button click
						numberOfLaneButtonClicks=0; //reset also the counter of the number of consecutive clics :-)
					}


					if(numberOfLaneButtonClicks>=2){ //if double click
						numberOfLaneButtonClicks=0; //ensure we don't return here :-)
						//notify to C2 and C1, that LANE was pressed 2 times (double tap)
						uint8_t tmpArr1[2]={C1_C2_BusID, C1_C2_cmdLaneDoubleTap};
						addToUARTSendQueue(tmpArr1, 2);
						onboardLed_blue_on();
					}
				}
			#endif
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
		case 0x000005A6:
			//leftHorizontal,leftVertical, rightHorizontal,rightVertical is from byte x (one byte each one)
			#if defined(BHbaccable)
				parkMirrorsSteady=((rx_msg_data[3] & 0x40)==0x00) && ((rx_msg_data[3] & 0x10)==0x00); //1 if side mirrors are not moving
				if(storeOperativeMirrorPosition ){ //if it was requested to store Operative side mirrors position
					if(parkMirrorsSteady){ //if mirror movement is not in progress
						storeOperativeMirrorPosition=0;
						if(parkMirrorOperativePositionNotStored){//if position was not previously stored, read it and store it
							parkMirrorOperativePositionNotStored=0;
							leftMirrorHorizontalOperativePos=	rx_msg_data[0];
							leftMirrorVerticalOperativePos=		rx_msg_data[1];
							rightMirrorHorizontalOperativePos=	rx_msg_data[2];
							rightMirrorVerticalOperativePos=	(rx_msg_data[3]<<4) | (rx_msg_data[4]>>4);
							saveOnFlashBH();
						}else{ //otherwise get it from memory
							leftMirrorHorizontalOperativePos	=(uint8_t)readFromFlashBH(6);
							leftMirrorVerticalOperativePos		=(uint8_t)readFromFlashBH(7);
							rightMirrorHorizontalOperativePos	=(uint8_t)readFromFlashBH(8);
							rightMirrorVerticalOperativePos 	=(uint8_t)readFromFlashBH(9);
						}
					}
				}
				if(storeCurrentParkMirrorPosition){
					if(parkMirrorsSteady){ //if mirror movement is not in progress
						storeCurrentParkMirrorPosition=0;
						leftParkMirrorHorizontalPos=	rx_msg_data[0];
						leftParkMirrorVerticalPos=		rx_msg_data[1];
						rightParkMirrorHorizontalPos=	rx_msg_data[2];
						rightParkMirrorVerticalPos=		(rx_msg_data[3]<<4) | (rx_msg_data[4]>>4);
						saveOnFlashBH(); //save it permanently on BH!
					}
				}

				if(restoreOperativeMirrorsPosition){ //if we are returning to original operative position
					//if we completed the return to the original operative position of the mirrors, set the variable as completed
					if(((rx_msg_data[3]>>4) & 0b00000101)==0x00){ //if mirrors are steady
						if( 			(rx_msg_data[0]>= leftMirrorHorizontalOperativePos	-1) && (rx_msg_data[0]<= leftMirrorHorizontalOperativePos	+1)){
							if( 		(rx_msg_data[1]>= leftMirrorVerticalOperativePos	-1) && (rx_msg_data[1]<= leftMirrorVerticalOperativePos		+1)){
								if(		(rx_msg_data[2]>= rightMirrorHorizontalOperativePos	-1) && (rx_msg_data[2]<= rightMirrorHorizontalOperativePos	+1)){
									if(	(((rx_msg_data[3]<<4) | (rx_msg_data[4]>>4))>= rightMirrorVerticalOperativePos-1) && (((rx_msg_data[3]<<4) | (rx_msg_data[4]>>4))<= rightMirrorVerticalOperativePos+1) ){
										restoreOperativeMirrorsPosition=0; //save the fact that the requested operation was successully completed
									}
								}
							}
						}
					}
				}
			#endif
			break;
		case 0x000005A8:
			//when in race, byte 4 bit 3-6 has value 6
			#if defined(C1baccable)
				if (ESCandTCinversion){
					if((rx_msg_data[4] & 0x78)!=0x30){  //if not race
						rx_msg_data[4] = (rx_msg_data[4] & ~0x78) | 0x30;  //set track mode (race)
						uint8_t tmpCounter=(rx_msg_data[6] & 0x0F)+1;
						if(tmpCounter>0x0F) tmpCounter=0;
						rx_msg_data[6]= (rx_msg_data[6] & 0xF0) | tmpCounter;   //increment counter
						rx_msg_data[7]=calculateCRC(rx_msg_data,rx_msg_header.DLC); //update CRC
						can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
					}
				}
			#endif

			#if defined(BHbaccable)
				if(function_park_mirror){
					//BH movement of mirror is requested
					if(leftParkMirrorPositionRequired || rightParkMirrorPositionRequired || restoreOperativeMirrorsPosition){ //if required
						if(!storeOperativeMirrorPosition && !parkMirrorsSteady){ //if Operative position was stored and mirror is not steady
							can_tx(&parkMirrorMsgHeader, parkMirrorMsgData); //send msg
						}
					}
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
							if((rx_msg_data[4] & 0x04 )==0){ //if the message needs to be changed
								#ifdef DPF_REGEN_VISUAL_ALERT
									//change message and send it again
									//memcpy(STATUS_ECM_msg_data, &rx_msg_data, rx_msg_header.DLC); //copy message
									//STATUS_ECM_msg_data[4] |= 0x04; //DPF Dirty (bit 2) set to ON
									//STATUS_ECM_msg_header.DLC=rx_msg_header.DLC;
									//can_tx(&STATUS_ECM_msg_header, STATUS_ECM_msg_data); //send msg
									rx_msg_data[4] |= 0x04; //DPF Dirty (bit 2) set to ON
									can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send msg
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
							loopsFromRegenerationEnded=0;
						}
						if((dieselEngineRegenerationMode==0) && (regenerationInProgress==1)){ //if regeneration has ended,
							loopsFromRegenerationEnded++;
							if (loopsFromRegenerationEnded>10){
								regenerationInProgress=0;
								loopsFromRegenerationEnded=0;
							}

						}else{
							loopsFromRegenerationEnded=0;
						}
					}

				}
			#endif
			break;
		case 0x000005B0:
			#if defined(C2baccable)

				if((rx_msg_data[1] == 0x20) && ( DynoStateMachine == 0xff)){ // park assist button was pressed and there is no dyno Start sequence in progress
					//HAS_buttonPressRequested=1;
					ParkAssistButtonPressCount++;
					if (ParkAssistButtonPressCount>5){ // more or less 6 seconds
						ParkAssistButtonPressCount=0; //reset the count
						dynoToggle();
						//wait the feedback from ECU
					}
				}else{
					ParkAssistButtonPressCount=0;// reset the count assigning it zero
					//HAS_buttonPressRequested=0;
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
