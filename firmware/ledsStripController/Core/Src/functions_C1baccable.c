/*
 * C1baccableFunctions.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 *      Common functions for C1baccable
 */
#include <functions_C1baccable.h>

#if defined(C1baccable)
	void C1baccableInitCheck(){
		lowConsume_init();
		immobilizerEnabled = (uint8_t)readFromFlash(1);  //parameter1 stored in ram, so that we can get it. By default Immo is enabled
		if(immobilizerEnabled) executeDashboardBlinks=2; //shows the user that the immobilizer is active (or not)
		function_smart_disable_start_stop_enabled=(uint8_t)readFromFlash(2);  //parameter2 stored in ram, so that we can get it. By default S&S is enabled
		function_led_strip_controller_enabled=(uint8_t)readFromFlash(3); //By default led is disabled
		function_shift_indicator_enabled=(uint8_t)readFromFlash(4); //By default it is disabled
		shift_threshold= (uint16_t)readFromFlash(5);
		function_ipc_my23_is_installed=	(uint8_t)readFromFlash(6);
		function_route_msg_enabled=(uint8_t)readFromFlash(7);
		function_dyno_mode_master_enabled=(uint8_t)readFromFlash(8);
		function_acc_virtual_pad_enabled=(uint8_t)readFromFlash(9);
		function_front_brake_forcer_master=(uint8_t)readFromFlash(10);
		function_4wd_disabler_enabled=(uint8_t)readFromFlash(11);
		function_remote_start_Enabled=(uint8_t)readFromFlash(12);
		function_clear_faults_enabled=(uint8_t)readFromFlash(13);
		function_esc_tc_customizator_enabled=(uint8_t)readFromFlash(14);
		function_read_faults_enabled=(uint8_t)readFromFlash(15);
		function_is_diesel_enabled=(uint8_t)readFromFlash(16);
		function_regeneration_alert_enabled=(uint8_t)readFromFlash(17);
		launch_torque_threshold= (uint16_t)readFromFlash(18);
		function_seatbelt_alarm_enabled= (uint16_t)readFromFlash(19);
		function_pedal_booster_enabled= (uint16_t)readFromFlash(20);
		uint8_t tmpArr1[3]={C2_Bh_BusID,C2_Bh_cmdSetPedalBoostStatus,function_pedal_booster_enabled};
		addToUARTSendQueue(tmpArr1, 3);

		function_disable_odometer_blink= (uint16_t)readFromFlash(21);
		//send the message to BH to inform about the status of the function disable_odometer_blink
		uint8_t tmpArr2[2]={BhBusID,BHcmdOdometerBlinkDefault};
		if(function_disable_odometer_blink) tmpArr2[1]=BHcmdOdometerBlinkDisable;
		addToUARTSendQueue(tmpArr2, 2);

		function_show_race_mask= (uint16_t)readFromFlash(22);
		//Now let's inform the C2 Baccable
		uint8_t tmpArr3[2]={C2BusID,C2cmdRaceMaskDefault};
		if(function_show_race_mask) tmpArr3[1]=C2cmdShowRaceMask;
		addToUARTSendQueue(tmpArr3, 2);

		function_park_mirror= (uint16_t)readFromFlash(23);
		//Now let's inform the BH Baccable
		uint8_t tmpArr4[2]={BhBusID,BHcmdFunctParkMirrorDisabled};
		if(function_park_mirror) tmpArr4[1]=BHcmdFunctParkMirrorEnabled;
		addToUARTSendQueue(tmpArr4, 2);
	}

	void C1baccablePeriodicCheck(){
		lowConsume_process();
		/*
		if(ESCandTCinversion){
			if((currentTime-lastSent384)>50){
				lastSent384=currentTime;
				DNA_msg_data[1] = (DNA_msg_data[1] & ~0x7C) | 0x30;  //set Race mode (0x30) to show on IPC the race screen
				uint8_t tmpCounter=(DNA_msg_data[6] & 0x0F)+1;
				if(tmpCounter>0x0F) tmpCounter=0;
				DNA_msg_data[6]= (DNA_msg_data[6] & 0xF0) | tmpCounter;   //increment counter
				DNA_msg_data[7]=calculateCRC(DNA_msg_data,DNA_msg_header.DLC); //update CRC
				can_tx((CAN_TxHeaderTypeDef *)&DNA_msg_header, DNA_msg_data); //transmit the modified packet
			}
		}
		*/

		if(instructSlaveBoardsTriggerEnabled){
			if((currentTime-allProcessorsWakeupTime)>5000){
				instructSlaveBoardsTriggerEnabled=0; //avoid to return here

				//send messages to slave boards
				uint8_t tmpArr1[3]={C2_Bh_BusID,C2_Bh_cmdSetPedalBoostStatus,function_pedal_booster_enabled};
				addToUARTSendQueue(tmpArr1, 3);

				//send the message to BH to inform about the status of the function disable_odometer_blink
				uint8_t tmpArr2[2]={BhBusID,BHcmdOdometerBlinkDefault};
				if(function_disable_odometer_blink) tmpArr2[1]=BHcmdOdometerBlinkDisable;
				addToUARTSendQueue(tmpArr2, 2);

				//Now let's inform the C2 Baccable
				uint8_t tmpArr3[2]={C2BusID,C2cmdRaceMaskDefault};
				if(function_show_race_mask) tmpArr3[1]=C2cmdShowRaceMask;
				addToUARTSendQueue(tmpArr3, 2);

				//Now let's inform the BH Baccable
				uint8_t tmpArr4[2]={BhBusID,BHcmdFunctParkMirrorDisabled};
				if(function_park_mirror) tmpArr4[1]=BHcmdFunctParkMirrorEnabled;
				addToUARTSendQueue(tmpArr4, 2);
			}
		}

		if(function_led_strip_controller_enabled==1){
			//don't act as canable. One USB port pin is used to control leds.
			vuMeterInit(); //initialize leds strip controller - this is called many times to divide the operations on more loops
			if(ledsStripIsOn){ //if the strip is on,
				if(currentTime-timeSinceLastReceivedAcceleratorMessage>10000 ){ //if no can interesting message for 10 seconds,  shutdown the leds to save energy
					shutdownLedsStrip();
					ledsStripIsOn=0; //entriamo solo una volta
				}
			}
		}

		if(function_4wd_disabler_enabled==1){
			if(_4wd_disabled>0){
				uint8_t tempDeltaTime=0;
				if(_4wd_disabled>1) tempDeltaTime=70;
				if(currentTime-last_sent_drive_train_msg_time>30+tempDeltaTime){ //enter here once each 100msec, then each 30msec
					last_sent_drive_train_msg_time=currentTime;
					onboardLed_blue_on();
					can_tx(&driveTrainControlModuleResetMsgHeader[_4wd_disabled-1], driveTrainControlModuleResetMsgData[_4wd_disabled-1]); //send drive train control module message
					if(_4wd_disabled>1) _4wd_disabled--;
				}
			}
		}

		if(executeDashboardBlinks>0){ //if we shall execute a blink to give a feedback to the user
			if(currentTime-last_sent_dashboard_blink_msg_time>500){ //enter here once each halfsecond
				last_sent_dashboard_blink_msg_time=currentTime;//return here after half second
				//change the message
				if(executeDashboardBlinks % 2==0){ //select one time reduced brightness and one time high brightness, so that in any condition the change is visible on the dashboard
					dashboardBlinkMsgData[4]=0x00; //max bright
				}else{
					dashboardBlinkMsgData[4]=0xF0; //reduced bright
				}
				executeDashboardBlinks = executeDashboardBlinks -1 ;//decrease blinks counter
				onboardLed_blue_on();
				//send the message
				can_tx(&dashboardBlinkMsgHeader, dashboardBlinkMsgData);
			}
		}


		if(immobilizerEnabled){
			//the following it is used only by IMMOBILIZER functionality
			if(floodTheBus){ //WHEN THIS IS ACTIVATED, THE THIEF WILL NOT BE ABLE TO CONNECT TO RFHUB, AND CAR WILL NOT SWITCH ON.
				if(currentTime-floodTheBusLastTimeSent>10){
					can_tx(&panicAlarmStartMsgHeader[0], panicAlarmStartMsgData[0]); //sends the message on can bus that resets the connection to RFHUB
					floodTheBusLastTimeSent=currentTime;
				}

				if(!panicAlarmActivated){ //if panic alarm is not activated, we shall activate it after 1 second (1 second to avoid stop and start simultaneous)
					if(currentTime-floodTheBusStartTime>1000){
						for (uint8_t i=0;i<15;i++){
							can_tx(&panicAlarmStartMsgHeader[1], panicAlarmStartMsgData[1]);
						}
						can_tx(&panicAlarmStartMsgHeader[2], panicAlarmStartMsgData[2]);
						panicAlarmActivated=1;
					}
				}
				if(currentTime-floodTheBusStartTime>10000){ //if the bus is flooded since 10 seconds, stop flooding it
					floodTheBus=0; //stop flooding
					//stop the panic alarm
					for (uint8_t i=0;i<15;i++){
						can_tx(&panicAlarmStartMsgHeader[1], panicAlarmStartMsgData[1]);
					}
					can_tx(&panicAlarmStartMsgHeader[2], panicAlarmStartMsgData[2]);
					panicAlarmActivated=0;
				}


			}
		}

		if(function_smart_disable_start_stop_enabled){
			if(startAndStopEnabled){
				if(currentTime>10000){ //first 10 seconds don't do anything to avoid to disturb other startup functions or immobilizer
					if(engineOnSinceMoreThan5seconds>=500){ //if motor is on since at least 5 seconds

						if(startAndstopCarStatus==0){//if start & stop was found disabled in car, we don't need to do anything. Avoid to enter here; We enter here in example if board is switched when the car is running and S&S was still manually disabled by the pilot
							startAndStopEnabled=0;
							requestToDisableStartAndStop=0;
						}else{
							if(lastTimeStartAndstopDisablerButtonPressed==0){ //first time we arrive here, go inside
								requestToDisableStartAndStop=1;
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
				can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the request
				seatbeltAlarmDisabled=0xfe; //status is in aquisition
				seatbeltAlarmStatusRequestTime=currentTime;
				last_sent_uds_parameter_request_Time=currentTime;
			}
		}
		*/

		if(((seatbeltAlarmDisabled==0xfe) || (seatbeltAlarmDisabled==0x10) || (seatbeltAlarmDisabled==0x20) || (seatbeltAlarmDisabled==0x11) || (seatbeltAlarmDisabled==0x21)) && (currentTime-seatbeltAlarmStatusRequestTime>10000)){ //if operations in progress but timeout was reached
			seatbeltAlarmDisabled=0xff;//return to unknown status
		}


		if(((seatbeltAlarmDisabled==1) || (seatbeltAlarmDisabled==0xff) ) && (function_seatbelt_alarm_enabled==1)){ //the alarm is disabled (or unknown) and the use wants to enable it
			if(engineOnSinceMoreThan5seconds>=200){
				//request to enable Seatbelt Alarm
				//send diag session request
				uds_parameter_request_msg_header.ExtId=0x18DA60F1;
				uds_parameter_request_msg_data[0]=0x02;
				uds_parameter_request_msg_data[1]=0x10;
				uds_parameter_request_msg_data[2]=0x03;
				uds_parameter_request_msg_header.DLC=3;
				can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the diag session request

				seatbeltAlarmDisabled=0x20; //request to enable SeatBelt alarm in progress(send write param)
				seatbeltAlarmStatusRequestTime=currentTime;
				last_sent_uds_parameter_request_Time=currentTime;
			}
		}

		if((seatbeltAlarmDisabled==0 || seatbeltAlarmDisabled==0xff)  && function_seatbelt_alarm_enabled==0){ //alarm is enabled (or unknown) and user wants to disable it
			if(engineOnSinceMoreThan5seconds>=200){
				//request to disable Seatbelt Alarm
				//send diag session request
				uds_parameter_request_msg_header.ExtId=0x18DA60F1;
				uds_parameter_request_msg_data[0]=0x02;
				uds_parameter_request_msg_data[1]=0x10;
				uds_parameter_request_msg_data[2]=0x03;
				uds_parameter_request_msg_header.DLC=3;
				can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the diag session request

				seatbeltAlarmDisabled=0x10; //request to disable SeatBelt alarm in progress(send write param)
				seatbeltAlarmStatusRequestTime=currentTime;
				last_sent_uds_parameter_request_Time=currentTime;
			}
		}


		if(shutdownDashboardMenuRequestTime>0){
			if(currentTime-shutdownDashboardMenuRequestTime>39000){
				baccableDashboardMenuVisible=0; //stop sending params request when motor is off
				clearDashboardBaccableMenu(); //ripulisci la stringa
				baccabledashboardMenuWasVisible=1; //allows the menu to automatically turn on when motor rotates
				shutdownDashboardMenuRequestTime=0; //avoid to return here
			}
		}
		//send a parameter request each xx msec if dashboard menu shall be visible
		//baccableDashboardMenuVisible=1; //force menu always on, just for debug
		if((currentTime-last_sent_uds_parameter_request_Time>500) && baccableDashboardMenuVisible ){
			last_sent_uds_parameter_request_Time=currentTime;

			switch(dashboard_menu_indent_level){
				case 0: //main menu
					sendMainDashboardPageToSlaveBaccable();
					break;
				case 1:
					if(main_dashboardPageIndex==1){ //we are in show params submenu
						clearFaultsRequest=0; //ensure we don't perform more tasks simoultaneously
						if(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId>0xFF){ //if req id is greather than 0xFF it is a standard UDS request.
							//request current parameter to ECU
							uds_parameter_request_msg_header.ExtId=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId;


							memcpy(&uds_parameter_request_msg_data[0],&uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqData,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqLen );
							uds_parameter_request_msg_header.DLC=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqLen;
							//uds_parameter_request_msg_header.ExtId=0x18DA40F1;
							//uds_parameter_request_msg_header.DLC=4;
							//uds_parameter_request_msg_data[0]=0x03;
							//uds_parameter_request_msg_data[1]=0x22;
							//uds_parameter_request_msg_data[2]=0x10;
							//uds_parameter_request_msg_data[3]=0x05;
							//onboardLed_blue_on();
							can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the request
						}else{ //0xff reqId is a special value that we use to send particular values. now we use to send baccable FW version and oil pressure, for now
							sendDashboardPageToSlaveBaccable(-3400000000000000000);
						}
					}

					if(main_dashboardPageIndex==9){
						sendSetupDashboardPageToSlaveBaccable();
					}
					break;
				default:
					break; //unexpected
			}
		}
	}

	void sendMainDashboardPageToSlaveBaccable(){
		uint8_t tmpStrLen=0;
		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus
		char tmpfloatString[5]; //temp array
		//update records if required
		switch(main_dashboardPageIndex){
			case 2: //READ_FAULTS_ENABLED
				if(function_read_faults_enabled==1){

				}
				break;
			case 3:
				if(function_clear_faults_enabled==1){
					if(clearFaultsRequest>0){
						memcpy(dashboard_main_menu_array[main_dashboardPageIndex], "WAIT...     ", 12);
						commandsMenuEnabled=0; //disable menu movement
					}else{
						memcpy(dashboard_main_menu_array[main_dashboardPageIndex], "CLEAR FAULTS", 12);
						commandsMenuEnabled=1; //enable menu movement
					}
				}
				break;
			case 4: //immo
				if(immobilizerEnabled){
					dashboard_main_menu_array[main_dashboardPageIndex][14]='N'; //on
					dashboard_main_menu_array[main_dashboardPageIndex][15]=' ';
				}else{
					dashboard_main_menu_array[main_dashboardPageIndex][14]='F'; //off
					dashboard_main_menu_array[main_dashboardPageIndex][15]='F';
				}
				break;
			case 5: //dyno
				if(printStopTheCar>0){
					printStopTheCar--;
					uint8_t stopTheCarMsg[13]={BhBusIDparamString,'S','T','O','P',' ','T','H','E',' ','C','A','R'};
					addToUARTSendQueue(stopTheCarMsg, 13);//print message "stop the car"
					return;
				}
				break;
			case 6: //ESC/TC
				break;
			case 7: //front brake
				if(printStopTheCar>0){
					printStopTheCar--;
					uint8_t stopTheCarMsg[13]={BhBusIDparamString,'S','T','O','P',' ','T','H','E',' ','C','A','R'};
					addToUARTSendQueue(stopTheCarMsg, 13);//print message "stop the car"
					return;
				}

				if(printEnableDyno>0){
					printEnableDyno--;
					uint8_t enableDynoMsg[12]={BhBusIDparamString,'E','N','A','B','L','E',' ','D','Y','N','O'};
					addToUARTSendQueue(enableDynoMsg, 12);//print message "Enable Dyno"
					return;
				}
				//update the string
				if(front_brake_forced==0){
					//update text {'F','r','o','n','t',' ','B','r','a','k','e',' ','N','o','r','m','a','l' },
					dashboard_main_menu_array[main_dashboardPageIndex][0]='F';
					dashboard_main_menu_array[main_dashboardPageIndex][1]='r';
					dashboard_main_menu_array[main_dashboardPageIndex][2]='o';
					dashboard_main_menu_array[main_dashboardPageIndex][3]='n';
					dashboard_main_menu_array[main_dashboardPageIndex][4]='t';
					dashboard_main_menu_array[main_dashboardPageIndex][5]=' ';
					dashboard_main_menu_array[main_dashboardPageIndex][6]='B';
					dashboard_main_menu_array[main_dashboardPageIndex][7]='r';
					dashboard_main_menu_array[main_dashboardPageIndex][8]='a';
					dashboard_main_menu_array[main_dashboardPageIndex][9]='k';
					dashboard_main_menu_array[main_dashboardPageIndex][10]='e';
					dashboard_main_menu_array[main_dashboardPageIndex][11]=' ';
					dashboard_main_menu_array[main_dashboardPageIndex][12]='N'; //normal
					dashboard_main_menu_array[main_dashboardPageIndex][13]='o'; //
					dashboard_main_menu_array[main_dashboardPageIndex][14]='r'; //
					dashboard_main_menu_array[main_dashboardPageIndex][15]='m';
					dashboard_main_menu_array[main_dashboardPageIndex][16]='a';
					dashboard_main_menu_array[main_dashboardPageIndex][17]='l';
				}else{
					if(launch_assist_enabled==1){
						if(torque>50){ //assist active and torque greather than minimum threshold. the minimum thr allows to view the menu "...assist" before the following string is printed
							//prepare variables

							//show torque
							dashboard_main_menu_array[main_dashboardPageIndex][0]='L';
							dashboard_main_menu_array[main_dashboardPageIndex][1]='a';
							dashboard_main_menu_array[main_dashboardPageIndex][2]='u';
							dashboard_main_menu_array[main_dashboardPageIndex][3]='n';
							dashboard_main_menu_array[main_dashboardPageIndex][4]='c';
							dashboard_main_menu_array[main_dashboardPageIndex][5]='h';
							dashboard_main_menu_array[main_dashboardPageIndex][6]=' ';

							floatToStr(tmpfloatString,(float)torque,0,4);
							switch(strlen(tmpfloatString)){
								case 1:
									dashboard_main_menu_array[main_dashboardPageIndex][7]=' ';
									dashboard_main_menu_array[main_dashboardPageIndex][8]=' ';
									dashboard_main_menu_array[main_dashboardPageIndex][9]=tmpfloatString[0];
									break;
								case 2:
									dashboard_main_menu_array[main_dashboardPageIndex][7]=' ';
									dashboard_main_menu_array[main_dashboardPageIndex][8]=tmpfloatString[0];
									dashboard_main_menu_array[main_dashboardPageIndex][9]=tmpfloatString[1];
									break;
								default:
									dashboard_main_menu_array[main_dashboardPageIndex][7]=tmpfloatString[0];
									dashboard_main_menu_array[main_dashboardPageIndex][8]=tmpfloatString[1];
									dashboard_main_menu_array[main_dashboardPageIndex][9]=tmpfloatString[2];
									break;
							}

							dashboard_main_menu_array[main_dashboardPageIndex][10]='N';
							dashboard_main_menu_array[main_dashboardPageIndex][11]='m';
							dashboard_main_menu_array[main_dashboardPageIndex][12]='/';

							floatToStr(tmpfloatString,(float)launch_torque_threshold,0,4);
							if(strlen(tmpfloatString)==2){
								dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
								dashboard_main_menu_array[main_dashboardPageIndex][14]=tmpfloatString[0];
								dashboard_main_menu_array[main_dashboardPageIndex][15]=tmpfloatString[1];
							}else{
								dashboard_main_menu_array[main_dashboardPageIndex][13]=tmpfloatString[0];
								dashboard_main_menu_array[main_dashboardPageIndex][14]=tmpfloatString[1];
								dashboard_main_menu_array[main_dashboardPageIndex][15]=tmpfloatString[2];
							}

							dashboard_main_menu_array[main_dashboardPageIndex][16]='N';
							dashboard_main_menu_array[main_dashboardPageIndex][17]='m';
						}else{
							//update text
							dashboard_main_menu_array[main_dashboardPageIndex][0]='F';
							dashboard_main_menu_array[main_dashboardPageIndex][1]='r';
							dashboard_main_menu_array[main_dashboardPageIndex][2]='o';
							dashboard_main_menu_array[main_dashboardPageIndex][3]='n';
							dashboard_main_menu_array[main_dashboardPageIndex][4]='t';
							dashboard_main_menu_array[main_dashboardPageIndex][5]=' ';
							dashboard_main_menu_array[main_dashboardPageIndex][6]='B';
							dashboard_main_menu_array[main_dashboardPageIndex][7]='r';
							dashboard_main_menu_array[main_dashboardPageIndex][8]='a';
							dashboard_main_menu_array[main_dashboardPageIndex][9]='k';
							dashboard_main_menu_array[main_dashboardPageIndex][10]='e';
							dashboard_main_menu_array[main_dashboardPageIndex][11]=' ';
							dashboard_main_menu_array[main_dashboardPageIndex][12]='A'; //assist (launch control)
							dashboard_main_menu_array[main_dashboardPageIndex][13]='s'; //
							dashboard_main_menu_array[main_dashboardPageIndex][14]='s'; //
							dashboard_main_menu_array[main_dashboardPageIndex][15]='i';
							dashboard_main_menu_array[main_dashboardPageIndex][16]='s';
							dashboard_main_menu_array[main_dashboardPageIndex][17]='t';
						}
					}else{ //launch assist not enabled
						//update text
						dashboard_main_menu_array[main_dashboardPageIndex][0]='F';
						dashboard_main_menu_array[main_dashboardPageIndex][1]='r';
						dashboard_main_menu_array[main_dashboardPageIndex][2]='o';
						dashboard_main_menu_array[main_dashboardPageIndex][3]='n';
						dashboard_main_menu_array[main_dashboardPageIndex][4]='t';
						dashboard_main_menu_array[main_dashboardPageIndex][5]=' ';
						dashboard_main_menu_array[main_dashboardPageIndex][6]='B';
						dashboard_main_menu_array[main_dashboardPageIndex][7]='r';
						dashboard_main_menu_array[main_dashboardPageIndex][8]='a';
						dashboard_main_menu_array[main_dashboardPageIndex][9]='k';
						dashboard_main_menu_array[main_dashboardPageIndex][10]='e';
						dashboard_main_menu_array[main_dashboardPageIndex][11]=' ';
						dashboard_main_menu_array[main_dashboardPageIndex][12]='F'; //forced
						dashboard_main_menu_array[main_dashboardPageIndex][13]='o'; //
						dashboard_main_menu_array[main_dashboardPageIndex][14]='r'; //
						dashboard_main_menu_array[main_dashboardPageIndex][15]='c';
						dashboard_main_menu_array[main_dashboardPageIndex][16]='e';
						dashboard_main_menu_array[main_dashboardPageIndex][17]='d';
					}
				}
				break;
			case 8: //4wd
				if(printStopTheCar>0){
					printStopTheCar--;
					uint8_t stopTheCarMsg[13]={BhBusIDparamString,'S','T','O','P',' ','T','H','E',' ','C','A','R'};
					addToUARTSendQueue(stopTheCarMsg, 13);//print message "stop the car"
					return;
				}
				//nothing to do
				break;
			default:
				//nothing to do
				break;
		}

		//add string to record
		switch(main_dashboardPageIndex){
			case 0:
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				if (tmpStrLen < DASHBOARD_MESSAGE_MAX_LENGTH) { //if required pad with spaces
					memset(&uartTxMsg[1+tmpStrLen], ' ', UART_BUFFER_SIZE-(1+tmpStrLen)); //set to zero remaining chars
				}
				break;
			default:
				memcpy(&uartTxMsg[1], dashboard_main_menu_array[main_dashboardPageIndex],UART_BUFFER_SIZE-1);
				break;
		}



		//send to slave baccable
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);

	}

	void sendSetupDashboardPageToSlaveBaccable(){
		//uint8_t tmpStrLen=0;
		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus

		char tmpfloatString[5]; //temp array for shift and launch threshods
		//update records if required
		switch(setup_dashboardPageIndex){
			case 0: //{'S','A','V','E','&','E','X','I','T',},
				break;
			case 1: //{'[',' ',']','S','t','a','r','t','&','S','t','o','p'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_smart_disable_start_stop_enabled];
				break;
			case 2: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-',},
				floatToStr(tmpfloatString,(float)launch_torque_threshold,0,4);
				if(strlen(tmpfloatString)==2){
					dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
					dashboard_setup_menu_array[setup_dashboardPageIndex][14]=tmpfloatString[0];
					dashboard_setup_menu_array[setup_dashboardPageIndex][15]=tmpfloatString[1];
				}else{
					dashboard_setup_menu_array[setup_dashboardPageIndex][13]=tmpfloatString[0];
					dashboard_setup_menu_array[setup_dashboardPageIndex][14]=tmpfloatString[1];
					dashboard_setup_menu_array[setup_dashboardPageIndex][15]=tmpfloatString[2];
				}


				break;
			case 3: //{'[',' ',']','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_led_strip_controller_enabled];
				break;
			case 4: //{'[',' ',']','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_shift_indicator_enabled];
				break;
			case 5: //{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',},
				floatToStr(tmpfloatString,(float)shift_threshold,0,5);
				dashboard_setup_menu_array[setup_dashboardPageIndex][10]=tmpfloatString[0];
				dashboard_setup_menu_array[setup_dashboardPageIndex][11]=tmpfloatString[1];
				dashboard_setup_menu_array[setup_dashboardPageIndex][12]=tmpfloatString[2];
				dashboard_setup_menu_array[setup_dashboardPageIndex][13]=tmpfloatString[3];
				break;
			case 6: //{'[',' ',']','M','y','2','3',' ','I','P','C', },
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_ipc_my23_is_installed];
				break;
			case 7: //{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_regeneration_alert_enabled];
				break;
			case 8: //{'O',' ',' ','S','e','a','t','b','e','l','t',' ','A','l','a','r','m',' '},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_seatbelt_alarm_enabled];
				break;
			case 9: //{'[',' ',']','R','o','u','t','e',' ','M','e','s','s','a','g','e','s', },
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_route_msg_enabled];
				break;
			case 10: //{'[',' ',']','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_esc_tc_customizator_enabled];
				break;
			case 11: //{'[',' ',']','D','y','n','o',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_dyno_mode_master_enabled];
				break;
			case 12: //{'[',' ',']','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_acc_virtual_pad_enabled];
				break;
			case 13: //{'[',' ',']','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_front_brake_forcer_master];
				break;
			case 14: //{'[',' ',']','4','W','D',' ','D','i','s','a','b','l','e','r',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_4wd_disabler_enabled];
				break;
			case 15: //{'[',' ',']','C','l','e','a','r',' ','F','a','u','l','t','s',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_clear_faults_enabled];
				break;
			case 16: //{'[',' ',']','R','e','a','d',' ',' ','F','a','u','l','t','s',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_read_faults_enabled];
				break;
			case 17: //{'[',' ',']','R','e','m','o','t','e',' ','S','t','a','r','t',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_remote_start_Enabled];
				break;
			case 18: //{'Ø',' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
				if(function_is_diesel_enabled){
					dashboard_setup_menu_array[setup_dashboardPageIndex][3] ='D';
					dashboard_setup_menu_array[setup_dashboardPageIndex][4] ='i';
					dashboard_setup_menu_array[setup_dashboardPageIndex][5] ='e';
					dashboard_setup_menu_array[setup_dashboardPageIndex][6] ='s';
					dashboard_setup_menu_array[setup_dashboardPageIndex][7] ='e';
					dashboard_setup_menu_array[setup_dashboardPageIndex][8] ='l';
					dashboard_setup_menu_array[setup_dashboardPageIndex][9] =' ';
					dashboard_setup_menu_array[setup_dashboardPageIndex][10]=' ';
				}else{
					dashboard_setup_menu_array[setup_dashboardPageIndex][3] ='G';
					dashboard_setup_menu_array[setup_dashboardPageIndex][4] ='a';
					dashboard_setup_menu_array[setup_dashboardPageIndex][5] ='s';
					dashboard_setup_menu_array[setup_dashboardPageIndex][6] ='o';
					dashboard_setup_menu_array[setup_dashboardPageIndex][7] ='l';
					dashboard_setup_menu_array[setup_dashboardPageIndex][8] ='i';
					dashboard_setup_menu_array[setup_dashboardPageIndex][9] ='n';
					dashboard_setup_menu_array[setup_dashboardPageIndex][10]='e';
				}
				break;
			case 19: //{'[',' ',']','P','e','d','a','l',' ','B','o','o','s','t','e','r'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[!!function_pedal_booster_enabled]; // double ! coerces to bool
				switch(function_pedal_booster_enabled){
					case 0: //off
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='o';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]='o';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='s';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]='t';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='e';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='r';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					case 1: //Auto
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='A';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]='u';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='t';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='o';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					case 2: //Bypass
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='B';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]='y';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='p';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='a';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]='s';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]='s';
						break;
					case 3: //All Weather Map
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='A';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='M';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='a';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]='p';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					case 4: //Natural Map
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='N';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='M';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='a';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]='p';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					case 5: //Dynamic Map
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='D';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='M';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='a';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]='p';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					case 6: //Race Map
						dashboard_setup_menu_array[setup_dashboardPageIndex][10]='.';
						dashboard_setup_menu_array[setup_dashboardPageIndex][11]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][12]='R';
						dashboard_setup_menu_array[setup_dashboardPageIndex][13]=' ';
						dashboard_setup_menu_array[setup_dashboardPageIndex][14]='M';
						dashboard_setup_menu_array[setup_dashboardPageIndex][15]='a';
						dashboard_setup_menu_array[setup_dashboardPageIndex][16]='p';
						dashboard_setup_menu_array[setup_dashboardPageIndex][17]=' ';
						break;
					default: //we will never end here
						break;
				}
				break;
			case 20:
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_disable_odometer_blink];
				break;
			case 21:
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_show_race_mask];
				break;
			case 22:
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_park_mirror];
				break;
			default:
				break;
		}

		memcpy(&uartTxMsg[1], &dashboard_setup_menu_array[setup_dashboardPageIndex],UART_BUFFER_SIZE-1);
		//send to slave baccable
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);


	}

	void sendDashboardPageToSlaveBaccable(float param){
		uint8_t tmpStrLen=0;
		uint8_t tmpStrLen2=0;
		uint8_t tmpStrLen3=0;

		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus

		switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId){ //do preliminary additional stuff for special parameters (not uds)
			case 0x10: //print oil pressure
				param=(float) oilPressure * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale;
				break;
			case 0x11: //power in CV
				param=(float)torque * (float)currentRpmSpeed * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale ;
				break;
			case 0x12: //torque in NM
				param=(float)torque;
				break;
			case 0x13: //battery state of charge (%)
				param=(float)batteryStateOfCharge  * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale;
				break;
			case 0x14: //battery current (A)
				param=((float)batteryCurrent  * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x15: //engine oil temperature
				param=((float)oilTemperature * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x16: //transmission temperature
				param=((float)transmissionTemperature * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x17: //current gear
				param=(float)(currentGear>>4);
				break;
			case 0x18: //current speed (km/h)
				param=currentSpeed_km_h;
				break;
			case 0x1A: //stat 0-100km/h
				param=chronometerElapsedTime_0_100_km_h; // >=20=MISSED , increase in progress= GO
				break;
			case 0x1B: //stat 100-200km/h
				param=chronometerElapsedTime_100_200_km_h; // >=40=MISSED , increase in progress= GO
				break;
			case 0x1C: //Best stat 0-100km/h
				param=readStatisticsFromFlash(1);
				break;
			case 0x1D: //Best stat 100-200km/h
				param=readStatisticsFromFlash(2);
				break;
			case 0x1E: //seat belt alarm status
				param=seatbeltAlarmDisabled;
				break;
			default:
				break;
		}


		switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId){
			case 0: //print baccable menu
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				break;
			case 0x19: //if diesel engine mode status
				tmpStrLen=18;
				switch(dieselEngineRegenerationMode){
					case 0:
						memcpy(&uartTxMsg[1], "REGEN.: NONE      ", tmpStrLen);
						break;
					case 1:
						memcpy(&uartTxMsg[1], "REGEN.: DPF LO    ", tmpStrLen);
						break;
					case 2:
						memcpy(&uartTxMsg[1], "REGEN.: DPF HI    ", tmpStrLen);
						break;
					case 3:
						memcpy(&uartTxMsg[1], "REGEN.: NSC De-NOx", tmpStrLen);
						break;
					case 4:
						memcpy(&uartTxMsg[1], "REGEN.: NSC De-SOx", tmpStrLen);
						break;
					case 5:
						memcpy(&uartTxMsg[1], "REGEN.: SCR HeatUp", tmpStrLen);
						break;
					default:
						memcpy(&uartTxMsg[1], "REGEN.: NONE.     ", tmpStrLen);
						break;
				}
				break;
			case 0x1F: //debug string
				tmpStrLen=18;
				//dobbiamo stampare: stato di sleep degli altri due chip
				// 					 tempo da ultimo messaggio ricevuto su C1
				//					 tempo da ultimo intervento IMMO
				uint8_t debugString[18];
				char tmpfloatString3[10];

				if(lowConsumeIsActive){
					debugString[0]='O';
					debugString[1]='f';
					debugString[2]='f';

				}else{
					debugString[0]='O';
					debugString[1]='n';
					debugString[2]=' ';
				}
				debugString[3]=' ';
				debugString[4]='T';
				debugString[5]='w';
				debugString[6]='k';
				debugString[7]='p';



				float allProcessorsWakeupElapsedTime=((float)currentTime-(float)allProcessorsWakeupTime)/(float)1000.0;
				if(allProcessorsWakeupElapsedTime>999.0){
					debugString[8]= '>';
					debugString[9]= '9';
					debugString[10]='9';
				}else{
					//scriviamo il valore
					floatToStr(tmpfloatString3,(float)allProcessorsWakeupElapsedTime, 0,4);
					memcpy(&debugString[8],tmpfloatString3,strlen(tmpfloatString3));
					if(strlen(tmpfloatString3)<3) debugString[10]=' ';
					if(strlen(tmpfloatString3)<2) debugString[9] =' ';

				}
				debugString[11]=' ';
				debugString[12]='I';
				debugString[13]='m';
				debugString[14]='m';
				float tmpLastImmoElapsedTime=((float)currentTime-(float)floodTheBusStartTime)/(float)1000.0;
				if(tmpLastImmoElapsedTime>999.0){
					debugString[15]= '>';
					debugString[16]= '9';
					debugString[17]= '9';
				}else{
					//scriviamo il valore
					floatToStr(tmpfloatString3,(float)tmpLastImmoElapsedTime,0,4);
					memcpy(&debugString[15],tmpfloatString3,strlen(tmpfloatString3));
					if(strlen(tmpfloatString3)<3) debugString[17]=' ';
					if(strlen(tmpfloatString3)<2) debugString[16]=' ';
				}


				memcpy(&uartTxMsg[1],debugString,18);
				break;
			default:
				tmpStrLen=strlen((const char *)uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH; //truncate it. no space left
				memcpy(&uartTxMsg[1], &uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name,tmpStrLen); //prepare name of parameter
				if(param!=-3400000000000000000){ //if different than special value (since special value means no value to send)
					//scale param still done, we don't need to do it here

					//convert param from float to string
					char tmpfloatString[10];

					switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId) {
						case 0x17: //if Current gear request data - currentGear
							if ((uint8_t)param<11){
								tmpfloatString[0]=gearArray[(uint8_t)param];
							}else{
								tmpfloatString[0] = '-';
							}

							tmpfloatString[1] =0;
							break;
						case 0x1A: //0-100 stat
							if(param>20.0){ //missed
								tmpfloatString[0]='M';
								tmpfloatString[1]='I';
								tmpfloatString[2]='S';
								tmpfloatString[3]='S';
								tmpfloatString[4]='E';
								tmpfloatString[5]='D';
								tmpfloatString[6]=' ';
								tmpfloatString[7]=0;
							}else{
								if(statistics_0_100_started){
									tmpfloatString[0]='G';
									tmpfloatString[1]='O';
									tmpfloatString[2]=' ';
									tmpfloatString[3]=' ';
									tmpfloatString[4]=' ';
									tmpfloatString[5]=' ';
									tmpfloatString[6]=' ';
									tmpfloatString[7]=0;
								}else{ //show time
									floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
								}
							}
							break;
						case 0x1B: //100-200 stat
							if(param>40.0){ //missed
								tmpfloatString[0]='M';
								tmpfloatString[1]='I';
								tmpfloatString[2]='S';
								tmpfloatString[3]='S';
								tmpfloatString[4]='E';
								tmpfloatString[5]='D';
								tmpfloatString[6]=0;
							}else{
								if(statistics_100_200_started){
									tmpfloatString[0]='G';
									tmpfloatString[1]='O';
									tmpfloatString[2]=' ';
									tmpfloatString[3]=' ';
									tmpfloatString[4]=' ';
									tmpfloatString[5]=' ';
									tmpfloatString[6]=0;
								}else{ //show time
									floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
								}
							}
							break;
						case 0x1C: //Best 0-100
							if(param>20.0){ //missed
								tmpfloatString[0]='M';
								tmpfloatString[1]='I';
								tmpfloatString[2]='S';
								tmpfloatString[3]='S';
								tmpfloatString[4]='E';
								tmpfloatString[5]='D';
								tmpfloatString[6]=' ';
								tmpfloatString[7]=0;
							}else{
								floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
							}
							break;
						case 0x1D: //Best 100-200
							if(param>40.0){ //missed
								tmpfloatString[0]='M';
								tmpfloatString[1]='I';
								tmpfloatString[2]='S';
								tmpfloatString[3]='S';
								tmpfloatString[4]='E';
								tmpfloatString[5]='D';
								tmpfloatString[6]=' ';
								tmpfloatString[7]=0;
							}else{
								floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
							}
							break;
						case 0x1E: //seat belt alarm status
							//we use an enumerated
							switch (seatbeltAlarmDisabled){
								case 1:
									tmpfloatString[0]='O';
									tmpfloatString[1]='F';
									tmpfloatString[2]='F';
									tmpfloatString[3]=0;
									break;
								case 0:
									tmpfloatString[0]='O';
									tmpfloatString[1]='N';
									tmpfloatString[2]=0;
									break;
								default:
									tmpfloatString[0]='?';
									tmpfloatString[1]=0;
							}

							break;
						default:
							floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
					}

					//add param to the page String
					tmpStrLen2=strlen(tmpfloatString);
					if(tmpStrLen+tmpStrLen2>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen2=DASHBOARD_MESSAGE_MAX_LENGTH-tmpStrLen; //truncate it. no space left
					memcpy(&uartTxMsg[1+tmpStrLen],tmpfloatString,tmpStrLen2);

					//float tmpVal9=200000.45601928209374; ///ADDED FOR TEST.......
					//char *tmpStr9=(char*)malloc(10);

					//floatToStr(tmpfloatString,param,2,sizeof(tmpfloatString));
					//tmpStrLen2=strlen(tmpfloatString);
					//memcpy(&dashboardPageStringArray[tmpStrLen],tmpfloatString,tmpStrLen2);
				}
				//add measurement unit
				tmpStrLen3=strlen((const char *)uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyMeasurementUnit);
				if(tmpStrLen+tmpStrLen2+tmpStrLen3>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen3=DASHBOARD_MESSAGE_MAX_LENGTH-tmpStrLen-tmpStrLen2; //truncate it. no space left
				memcpy(&uartTxMsg[1+tmpStrLen+tmpStrLen2],&uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyMeasurementUnit,tmpStrLen3);
		}
		if (tmpStrLen+tmpStrLen2+tmpStrLen3 < DASHBOARD_MESSAGE_MAX_LENGTH) { //if required pad with zeros
			memset(&uartTxMsg[1+tmpStrLen+tmpStrLen2+tmpStrLen3], ' ', UART_BUFFER_SIZE-(1+tmpStrLen+tmpStrLen2+tmpStrLen3)); //set to zero remaining chars
		}
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);


	}

	void clearDashboardBaccableMenu(){
		//prepare empty message
		uartTxMsg[0]=BhBusIDparamString; //# to send message to baccable slave connected to BH can bus
		for(uint8_t i=1;i<UART_BUFFER_SIZE;i++){
			uartTxMsg[i]=0x20; //space char
		}
		//send it
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);

	}

	// this function scales value received from can bus. It is assumed that pedal position (or motor rpm) will change vumeter volume represented with the leds strip
	float scaleVolume(uint8_t vol){
		//Scale this value to get a percentage between 0 and 100
		//sniffed data on can bus: msg id=0x412 , fourth byte goes from 0x33(51) to 0xE6(230)
		if (vol>50){
			vol=vol-51;
		}
		return (float)(((float)vol*100.0f)/180.0f);
	}

	// this function scales value received from can bus. It is assumed that gear selection will change the color effect of the leds strip
	uint8_t scaleColorSet(uint8_t col){
		//id 2ef, primo byte, 70=r, 00=n, f0=marcia inserita ma frizione premuta (indefinito), 10=prima, 20=seconda..
		col= col>>4;
		//onboardLed_red_blink(col);
		// 7=backward, f=gear set but frizione premuta (undefined), 1=first gear , 2=second gear, ... , 6= sixt gear
		return col;
	}

	uint8_t saveOnflash(){ //store params permanently on flash
		//last page to store on flash is 0x0801 F800 (we can store 2 bytes each time)
		// and we shall erase entire page before write. one page size is FLASH_PAGE_SIZE (2048 bytes in st32F072)
		HAL_FLASH_Unlock(); //unlock flash

		//erase flash
		FLASH_EraseInitTypeDef eraseInitStruct;
		uint32_t pageError=0;
		eraseInitStruct.TypeErase= FLASH_TYPEERASE_PAGES;
		eraseInitStruct.PageAddress=LAST_PAGE_ADDRESS; //last page address begin
		eraseInitStruct.NbPages=1;
		if(HAL_FLASHEx_Erase(&eraseInitStruct,&pageError)!=HAL_OK){ //error during erase
			HAL_FLASH_Lock();
			onboardLed_red_blink(8);
			return 254; //error
		}

		//it seems that stm32F072 supports only writing 2byte words
		//write parameter
		uint8_t paramsNumber=23;
		uint16_t params[40] = {
		  immobilizerEnabled,
		  function_smart_disable_start_stop_enabled,
		  function_led_strip_controller_enabled,
		  function_shift_indicator_enabled,
		  shift_threshold,
		  function_ipc_my23_is_installed,
		  function_route_msg_enabled,
		  function_dyno_mode_master_enabled,
		  function_acc_virtual_pad_enabled,
		  function_front_brake_forcer_master,
		  function_4wd_disabler_enabled,
		  function_remote_start_Enabled,
		  function_clear_faults_enabled,
		  function_esc_tc_customizator_enabled,
		  function_read_faults_enabled,
		  function_is_diesel_enabled,
		  function_regeneration_alert_enabled,
		  launch_torque_threshold,
		  function_seatbelt_alarm_enabled,
		  function_pedal_booster_enabled,
		  function_disable_odometer_blink,
		  function_show_race_mask,
		  function_park_mirror,
		};

		for (uint8_t i = 0; i < paramsNumber; i++) {
		    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, LAST_PAGE_ADDRESS + (i * 4), params[i]) != HAL_OK) {
		        HAL_FLASH_Lock();
		        onboardLed_red_blink(9);
		        return 255;
		    }
		}

		//lock the flash
		HAL_FLASH_Lock();
		return 0;
	}

	uint8_t saveStatisticsOnFlash(){
		//compare current stats with best stored times
		uint8_t weShallWriteFlash=0;
		uint16_t params[10];
		if(chronometerElapsedTime_0_100_km_h<readStatisticsFromFlash(1)){
			params[0]=(uint16_t)(chronometerElapsedTime_0_100_km_h *1000);
			weShallWriteFlash=1;
		}else{
			params[0]=(uint16_t)(readStatisticsFromFlash(1) * 1000);
		}

		if(chronometerElapsedTime_100_200_km_h<readStatisticsFromFlash(2)){
			params[1]=(uint16_t)(chronometerElapsedTime_100_200_km_h *1000);
			weShallWriteFlash=1;
		}else{
			params[1]=(uint16_t)(readStatisticsFromFlash(2) * 1000);
		}


		if(weShallWriteFlash){ //if new best time was found, save again all stats
			HAL_FLASH_Unlock(); //unlock flash

			//erase flash
			FLASH_EraseInitTypeDef eraseInitStruct;
			uint32_t pageError=0;
			eraseInitStruct.TypeErase= FLASH_TYPEERASE_PAGES;
			eraseInitStruct.PageAddress=LAST_PAGE_ADDRESS_STATISTICS;
			eraseInitStruct.NbPages=1;
			if(HAL_FLASHEx_Erase(&eraseInitStruct,&pageError)!=HAL_OK){ //error during erase
				HAL_FLASH_Lock();
				onboardLed_red_blink(8);
				return 254; //error
			}

			//it seems that stm32F072 supports only writing 2byte words
			//write parameter

			for (uint8_t i = 0; i < 9; i++) {
				if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, LAST_PAGE_ADDRESS_STATISTICS + (i * 4), params[i]) != HAL_OK) {
					HAL_FLASH_Lock();
					onboardLed_red_blink(9);
					return 255;
				}
			}

			//lock the flash
			HAL_FLASH_Lock();
			return 0;

		}
		return 253;
	}

	float readStatisticsFromFlash(uint8_t paramId){ // 1=0-100km/h, 2=100/200km/h
		if(paramId<1) return 0;
		uint16_t tmpParam=*(volatile uint16_t*)(LAST_PAGE_ADDRESS_STATISTICS+((paramId-1)*4));

		return (float)(((float)tmpParam)/1000.0);

	}

	uint16_t readFromFlash(uint8_t paramId){
		if(paramId<1) return 0;
		uint16_t tmpParam=*(volatile uint16_t*)(LAST_PAGE_ADDRESS+((paramId-1)*4));
		switch(paramId){
			case 1: //IMMOBILIZER_ENABLED (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(IMMOBILIZER_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 2: //SMART_DISABLE_START_STOP enable status (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(SMART_DISABLE_START_STOP)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 3: //LED_STRIP_CONTROLLER_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(LED_STRIP_CONTROLLER_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 4: //SHIFT_INDICATOR_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(SHIFT_INDICATOR_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 5: //SHIFT_THRESHOLD
				if(tmpParam==0xFFFF){
					#if defined(SHIFT_THRESHOLD)
						return SHIFT_THRESHOLD;
					#else
						return 3500; // another default value
					#endif
				}
				break;
			case 6: //IPC_MY23_IS_INSTALLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(IPC_MY23_IS_INSTALLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 7: //ROUTE_MSG  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ROUTE_MSG)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 8: //DYNO_MODE_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(DYNO_MODE_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 9: //ACC_VIRTUAL_PAD  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ACC_VIRTUAL_PAD)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 10: //FRONT_BRAKE_FORCER_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(FRONT_BRAKE_FORCER_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 11: //_4WD_DISABLER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(_4WD_DISABLER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 12: //REMOTE_START_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(REMOTE_START_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 13: //CLEAR_FAULTS_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(CLEAR_FAULTS_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 14: //ESC_TC_CUSTOMIZATOR_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ESC_TC_CUSTOMIZATOR_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 15: //READ_FAULTS_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(READ_FAULTS_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 16: //IS_DIESEL  (1=enabled 0=disabled)
					if(tmpParam>1){
						#if defined(IS_DIESEL)
							return 1;
						#else
							return 0;
						#endif
					}
					break;
			case 17: //REGENERATION_ALERT_ENABLED  (1=enabled 0=disabled)
					if(tmpParam>1){
						#if defined(REGENERATION_ALERT_ENABLED)
							return 1;
						#else
							return 0;
						#endif
					}
					break;
			case 18: //LAUNCH_THRESHOLD
				if(tmpParam==0xFFFF){
					#if defined(LAUNCH_THRESHOLD)
						return LAUNCH_THRESHOLD;
					#else
						return 100; // another default value
					#endif
				}
				break;
			case 19: //SEATBELT_ALARM_DISABLED
				if(tmpParam>1){
					#if defined(SEATBELT_ALARM_DISABLED)
						return 0;
					#else
						return 1;
					#endif
				}
				break;
			case 20: //PEDAL_BOOSTER_ENABLED
				if(tmpParam>6){
					#if defined(PEDAL_BOOSTER_ENABLED)
						return PEDAL_BOOSTER_ENABLED;
					#else
						return 0;
					#endif
				}
				break;
			case 21: //DISABLE_ODOMETER_BLINK
				if(tmpParam>1){
					#if defined(DISABLE_ODOMETER_BLINK)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 22: //SHOW_RACE_MASK
				if(tmpParam>1){
					#if defined(SHOW_RACE_MASK)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 23: //PARK_MIRROR
				if(tmpParam>1){
					#if defined(PARK_MIRROR)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			default:
				return 0;
				break;
		}
		return tmpParam;
	}

	uint8_t getParamIndexFromReqId(uint32_t searchedReqId){
		uint8_t totalParams=total_pages_in_dashboard_menu_gasoline;
		if(function_is_diesel_enabled) totalParams=total_pages_in_dashboard_menu_diesel;
		for (uint8_t i=0;i<totalParams;i++){
			if(uds_params_array[function_is_diesel_enabled][i].reqId==searchedReqId) return i;
		}
		return 0; //means not found, or param0 (it could be an exception)
	}

#endif
