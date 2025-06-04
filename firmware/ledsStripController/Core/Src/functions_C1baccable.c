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
	}

	void C1baccablePeriodicCheck(){
		lowConsume_process();

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
			case 8: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-', },
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
			default:
				break;
		}


		switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId){
			case 0: //print baccable menu
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				break;
			case 0x19:
				if (uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId == 0x19) { //if diesel engine mode status
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
				}
				break;
			default:
				tmpStrLen=strlen((const char *)uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH; //truncate it. no space left
				memcpy(&uartTxMsg[1], &uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name,tmpStrLen); //prepare name of parameter
				if(param!=-3400000000000000000){ //if different than special value (since special value means no value to send)
					//scale param still done, we don't need to do it here

					//convert param from float to string
					char tmpfloatString[10];

					if (uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId == 0x17) { //if Current gear request data - currentGear

						if ((uint8_t)param<11){
							tmpfloatString[0]=gearArray[(uint8_t)param];
						}else{
							tmpfloatString[0] = '-';
						}

						tmpfloatString[1] =0;
					}else{
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
		uint16_t params[20] = {
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
		};

		for (uint8_t i = 0; i < 18; i++) {
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
			default:
				return 0;
				break;
		}
		return tmpParam;
	}

#endif
