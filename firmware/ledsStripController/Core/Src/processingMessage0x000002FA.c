/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "debug.h"
#include "processingMessage0x000002FA.h"

void processingMessage0x000002FA(){
	// Button is pressed on left area of the wheel
	// These Buttons are detected only if the main panel of the car is on.

	#if defined(C1baccable)

		if(function_acc_autostart){
			if(ACC_engaged){
				if(currentSpeed_km_h==0){ //if car is steady
					if(rx_msg_data[0]==0x10){ //if no button was pressed on cruise control pad
						if (rx_msg_data[1] == 0x0A){ //once each 320msec (byte1 low nibble contains a counter from 0 to F)
							rx_msg_data[0] = 0x90; //Ress button press
							rx_msg_data[1] = 0x0B; //counter
							rx_msg_data[2] = 0x0C; //CRC
							can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send message to simulate RES button press
							rx_msg_data[0]=0x10; //restore value 10 to avoid unwanted behaviours in case function_acc_virtual_pad_enabled=1 (look next lines to understand)
						}
					}
				}
			}

	    }


	  	//function ACC Virtual Pad

		if(function_acc_virtual_pad_enabled==1){
			switch (rx_msg_data[0]){
				case 0x12: //CC on
					newWheelPressedButtonID=0x12;
					memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
					ACC_msg_data[0] = 0x11; //ACC On
					ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
					ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
					can_tx(&ACC_msg_header, ACC_msg_data); //send msg
					onboardLed_blue_on();
					break;
				case 0x90: //RES pressed
					if (newWheelPressedButtonID==0x10 && ACC_engaged) ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=1; //if begin to press button RES and ACC is Engaged, set ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED
					newWheelPressedButtonID=0x90; //store the new RES button status (pressed)
					if (ACC_engaged && ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED){
						//simulate the distance button press
						memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
						ACC_msg_data[0] = 0x50; //ACC distance change
						ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
						ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
						can_tx(&ACC_msg_header, ACC_msg_data); //send msg
						onboardLed_blue_on();
					}
					break;
				case 0x10: //button released
					newWheelPressedButtonID=0x10; //button released (I use another variable to distinguish from the one used in show params function
					ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=0;
					break;
				default:
			}
		}

		if(cruiseControlDisabled && ACC_Disabled){ //if we are allowed to press buttons, use them in baccable menu
			switch(rx_msg_data[0]){
				case 0x18://if cruise control speed reduction button was pressed, user wants to see next page
					if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
						wheelPressedButtonID=0x18; //avoid to return here
						if(commandsMenuEnabled){
							switch(dashboard_menu_indent_level){
								case 0: //main menu
									main_dashboardPageIndex+= 1; //set next page

									if(function_read_faults_enabled==0){
										if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
									}


									if(function_clear_faults_enabled==0){
										if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
									}

									if(function_dyno_mode_master_enabled==0){
										if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
									}

									if(function_esc_tc_customizator_enabled==0){
										if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
									}
									if(function_front_brake_forcer_master==0){
										if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
									}

									if(function_4wd_disabler_enabled==0){
										if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
									}

									if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
									//onboardLed_blue_on();
									sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
									break;
								case 1:
									if(main_dashboardPageIndex==1){ //we are in show params submenu
										dashboardPageIndex += 1; //set next page
										if(function_is_diesel_enabled==1){
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
										}else{
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
										}

										dashboardPageIndex=getNextVisibleParam(dashboardPageIndex);
										//onboardLed_blue_on();
										sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
									}
									if(main_dashboardPageIndex==9){ //we are in setup menu
										setup_dashboardPageIndex+=1;//set next page
										//if(setup_dashboardPageIndex==2) setup_dashboardPageIndex++; //future growth

										//if(setup_dashboardPageIndex==8) setup_dashboardPageIndex++; //future growth

										if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu

										//onboardLed_blue_on();
										sendSetupDashboardPageToSlaveBaccable(); //send
									}
									if(main_dashboardPageIndex==10){ //we are in params setup menu
										params_setup_dashboardPageIndex+=1;//set next page
										total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_gasoline;
										if(function_is_diesel_enabled) total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_diesel;

										if(params_setup_dashboardPageIndex>=total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
										sendParamsSetupDashboardPageToSlaveBaccable(); //send
									}
									break;
								default:
									break; //unexpected
							}
						}
					}
					break;
				case 0x20://if cruise control speed strong reduction button was pressed, user wants to jump 10 pages forward
						if(wheelPressedButtonID==0x18 && baccableDashboardMenuVisible){ //if button released, use pressed button
							wheelPressedButtonID=0x20; //avoid to return here
							if(commandsMenuEnabled){
								switch(dashboard_menu_indent_level){
									case 0: //main menu
										main_dashboardPageIndex+= 1; //set next page
										if(function_read_faults_enabled==0){
											if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
										}
										if(function_clear_faults_enabled==0){
											if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
										}

										if(function_dyno_mode_master_enabled==0){
											if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
										}
										if(function_esc_tc_customizator_enabled==0){
											if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
										}
										if(function_front_brake_forcer_master==0){
											if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
										}

										if(function_4wd_disabler_enabled==0){
											if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
										}

										if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
										//onboardLed_blue_on();
										sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
										break;
									case 1:
										if(main_dashboardPageIndex==1){ //we are in show params submenu
											dashboardPageIndex += 10; //set 10 pages forward (+1 in gentle command)
											if(function_is_diesel_enabled==1){
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
											}else{
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
											}
											dashboardPageIndex=getNextVisibleParam(dashboardPageIndex);
												//onboardLed_blue_on();
											sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
										}
										if(main_dashboardPageIndex==9){ //we are in setup menu
											setup_dashboardPageIndex+=10;//set next page
											//if(setup_dashboardPageIndex==2) setup_dashboardPageIndex++; //future growth
											//if(setup_dashboardPageIndex==8) setup_dashboardPageIndex++; //future growth
											if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendSetupDashboardPageToSlaveBaccable(); //send
										}
										if(main_dashboardPageIndex==10){ //we are in params setup menu
											params_setup_dashboardPageIndex+=10;//set next page
											total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_gasoline;
											if(function_is_diesel_enabled) total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_diesel;

											if(params_setup_dashboardPageIndex>=total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendParamsSetupDashboardPageToSlaveBaccable(); //send
										}
										break;
									default:
										break; //unexpected
								}
							}
						}

						break;
				case 0x08: //if cruise control speed increase button was pressed, user wants to see previous page
					if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
						wheelPressedButtonID=0x08; //avoid to enter again here
						if(commandsMenuEnabled){
							switch(dashboard_menu_indent_level){
								case 0: //main menu
									main_dashboardPageIndex-= 1; //set next page

									if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu
									if(function_4wd_disabler_enabled==0){
										if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
										}

									if(function_front_brake_forcer_master==0){
										if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
									}
									if(function_esc_tc_customizator_enabled==0){
										if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
									}
									if(function_dyno_mode_master_enabled==0){
										if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
									}

									if(function_clear_faults_enabled==0){
										if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
									}
									if(function_read_faults_enabled==0){
										if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
									}
									//onboardLed_blue_on();
									sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
									break;
								case 1:
									if(main_dashboardPageIndex==1){ //we are in show params submenu
										dashboardPageIndex -= 1; //set previous page
										if(function_is_diesel_enabled==1){
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=total_pages_in_dashboard_menu_diesel-1; // make a rotative menu
										}else{
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=total_pages_in_dashboard_menu_gasoline-1; // make a rotative menu
										}
										dashboardPageIndex=getPreviousVisibleParam(dashboardPageIndex);
										//onboardLed_blue_on();
										sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
									}
									if(main_dashboardPageIndex==9){ //we are in setup menu
										setup_dashboardPageIndex-=1;//set next page
										//if(setup_dashboardPageIndex==8) setup_dashboardPageIndex--; //future growth
										//if(setup_dashboardPageIndex==2) setup_dashboardPageIndex--; //future growth

										if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=total_pages_in_setup_dashboard_menu-1; // make a rotative menu
										//onboardLed_blue_on();
										sendSetupDashboardPageToSlaveBaccable(); //send
									}
									if(main_dashboardPageIndex==10){ //we are in params setup menu
										params_setup_dashboardPageIndex-=1;//set next page
										total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_gasoline;
										if(function_is_diesel_enabled) total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_diesel;

										if(params_setup_dashboardPageIndex>=total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=total_pages_in_params_setup_dashboard_menu-1; // make a rotative menu
										//onboardLed_blue_on();
										sendParamsSetupDashboardPageToSlaveBaccable(); //send
									}
									break;
								default:
									break; //unexpected
							}
						}
					}
					break;
				case 0x00: //if cruise control speed strong increase button was pressed, user wants to jump 10 pages before
						if(wheelPressedButtonID==0x08 && baccableDashboardMenuVisible){
							wheelPressedButtonID=0x00; //avoid to return here
							if(commandsMenuEnabled){
								switch(dashboard_menu_indent_level){
									case 0: //main menu
										main_dashboardPageIndex-= 1; //set next page
										if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu
										if(function_4wd_disabler_enabled==0){
											if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
										}
										if(function_front_brake_forcer_master==0){
											if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
										}
										if(function_esc_tc_customizator_enabled==0){
											if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
										}
										if(function_dyno_mode_master_enabled==0){
											if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
										}

										if(main_dashboardPageIndex==4) main_dashboardPageIndex--;

										if(function_clear_faults_enabled==0){
											if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
										}
										if(function_read_faults_enabled==0){
											if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
										}
										//onboardLed_blue_on();
										sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
										break;
									case 1:
										if(main_dashboardPageIndex==1){ //we are in show params submenu
											dashboardPageIndex -= 10; //set 10 pages backward

											if(function_is_diesel_enabled==1){
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // stay at zero.
											}else{
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // stay at zero.
											}
											dashboardPageIndex=getPreviousVisibleParam(dashboardPageIndex);


											//onboardLed_blue_on();
											sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
										}
										if(main_dashboardPageIndex==9){ //we are in setup menu
											setup_dashboardPageIndex-=10;//set prev page
											//if(setup_dashboardPageIndex==8) setup_dashboardPageIndex--; //future growth
											//if(setup_dashboardPageIndex==2) setup_dashboardPageIndex--; //future growth
											if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendSetupDashboardPageToSlaveBaccable(); //send
										}
										if(main_dashboardPageIndex==10){ //we are in params setup menu
											params_setup_dashboardPageIndex-=10;//set prev page
											total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_gasoline;
											if(function_is_diesel_enabled) total_pages_in_params_setup_dashboard_menu=total_pages_in_dashboard_menu_diesel;

											if(params_setup_dashboardPageIndex>=total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendParamsSetupDashboardPageToSlaveBaccable(); //send
										}
										break;
									default:
										break; //unexpected
								}
							}
						}
						break;
				case 0x10: // button released
					if(wheelPressedButtonID==0x89 && baccableDashboardMenuVisible==1){ //we pressed RES for at least one instant, then we released before 2 seconds, therefore we want to enter inside dashboard menu (will work only if menu is visible)

						if(dashboard_menu_indent_level==0){
							switch(main_dashboardPageIndex){
								case 1: //show params
									dashboard_menu_indent_level++;
									sendDashboardPageToSlaveBaccable(-3400000000000000000);
									break;
								case 2: //read faults
									//To Be Done
									break;
								case 3: //clear faults
									clearFaultsRequest=255;
									break;
								case 4: //Immo
									// nothing to do
									break;
								case 5: // toggle dyno status
									//send request thu serial line
									if(currentSpeed_km_h==0){
										uint8_t tmpArr1[2]={C2BusID,C2cmdtoggleDyno};
										addToUARTSendQueue(tmpArr1, 2);
									}else{
										printStopTheCar=2;//print message "stop the car"
									}
									break;
								case 6: //toggle ESC/TC
									//send request thu serial line
									uint8_t tmpArr2[2]={C2BusID,C2cmdtoggleEscTc};
									addToUARTSendQueue(tmpArr2, 2);
									break;
								case 7: //toggle front brake

									if(front_brake_forced>0){ //toggle front brake
										if(launch_assist_enabled==1){ //if assist is enabled (by default it is enabled)
											launch_assist_enabled=0; //disable assist
										}else{ //launch assist is not enabled, but brakes are forced
											//send serial message to C2 baccable, to RELEASE front brakes
											uint8_t tmpArr3[2]={C2BusID,C2cmdNormalFrontBrake};
											addToUARTSendQueue(tmpArr3, 2);
										}
									}else{
										if(currentSpeed_km_h==0){
											if(!DynoModeEnabledOnMaster){
												printEnableDyno=2; //print message Enable DYNO
											}else{
												/* This commented part is a test... not working now


												//if dyno function is enabled but dyno is not enabled, enable dyno
												if(function_dyno_mode_master_enabled){
													if(DynoModeEnabledOnMaster==0){
														//send serial message to C2 baccable, to enable dyno
														uint8_t tmpArr4[2]={C2BusID,C2cmdForceFrontBrake};
														addToUARTSendQueue(tmpArr4, 2);
													}
												}

												//if 4wd function is enabled but 4wd is not disabled, disable 4wd
												if(function_4wd_disabler_enabled){
													if(_4wd_disabled==0){
														//seend messages to disable	4wd
														//TBDone.......
													}
												}
												*/
												//send serial message to C2 baccable, to force front brakes
												uint8_t tmpArr5[2]={C2BusID,C2cmdForceFrontBrake};
												addToUARTSendQueue(tmpArr5, 2);
											}
										}else{
											printStopTheCar=2;//print message "stop the car"
										}
									}


									break;
								case 8: //toggle 4wd
									if(_4wd_disabled>0){ //toggle 4dw disable
										_4wd_disabled=0;
										//update text
										dashboard_main_menu_array[main_dashboardPageIndex][4]=' '; //enabled
										dashboard_main_menu_array[main_dashboardPageIndex][5]='E';
										dashboard_main_menu_array[main_dashboardPageIndex][6]='n';
										commandsMenuEnabled=1;//enable menu commands
									}else{
										if(currentSpeed_km_h==0){
											_4wd_disabled=4;
											//update text
											dashboard_main_menu_array[main_dashboardPageIndex][4]='D'; //disabled
											dashboard_main_menu_array[main_dashboardPageIndex][5]='i';
											dashboard_main_menu_array[main_dashboardPageIndex][6]='s';
											commandsMenuEnabled=0;//disable menu commands
										}else{
											printStopTheCar=2;//print message "stop the car"
										}
									}
									break;
								case 9: //setup menu
								case 10: //params setup menu
									dashboard_menu_indent_level++;

									break;
								default:
									break;
							}

						}else{ //indent level >0
							switch(main_dashboardPageIndex){
								case 9: //setup menu
									switch(setup_dashboardPageIndex){
										case 0: //{'S','A','V','E','&','E','X','I','T',},
											//if some change occurred
											if(	((uint16_t)function_smart_disable_start_stop_enabled	!= readFromFlash(2)) 	|| //S&S enable status
												((uint16_t)function_led_strip_controller_enabled		!= readFromFlash(3)) 	|| //LED_STRIP_CONTROLLER_ENABLED
												((uint16_t)function_shift_indicator_enabled				!= readFromFlash(4)) 	|| //SHIFT_INDICATOR_ENABLED
												((uint16_t)shift_threshold								!= readFromFlash(5))	|| //SHIFT_THRESHOLD
												((uint16_t)function_ipc_my23_is_installed				!= readFromFlash(6))	|| //IPC_MY23_IS_INSTALLED
												((uint16_t)function_route_msg_enabled					!= readFromFlash(7))	|| //ROUTE_MSG
												((uint16_t)function_dyno_mode_master_enabled			!= readFromFlash(8))	|| //DYNO_MODE_MASTER
												((uint16_t)function_acc_virtual_pad_enabled				!= readFromFlash(9))	|| //ACC_VIRTUAL_PAD
												((uint16_t)function_front_brake_forcer_master			!= readFromFlash(10))	|| //FRONT_BRAKE_FORCER_MASTER
												((uint16_t)function_4wd_disabler_enabled				!= readFromFlash(11))	|| //_4WD_DISABLER
												((uint16_t)function_remote_start_Enabled				!= readFromFlash(12))	|| //REMOTE_START_ENABLED
												((uint16_t)function_clear_faults_enabled				!= readFromFlash(13))	|| //CLEAR_FAULTS_ENABLED
												((uint16_t)function_esc_tc_customizator_enabled			!= readFromFlash(14))	|| //ESC_TC_CUSTOMIZATOR_MASTER
												((uint16_t)function_read_faults_enabled					!= readFromFlash(15))	|| //READ_FAULTS_ENABLED
												((uint16_t)function_is_diesel_enabled					!= readFromFlash(16))	|| //IS_DIESEL
												((uint16_t)function_regeneration_alert_enabled			!= readFromFlash(17))	|| //REGENERATION_ALERT_ENABLED
												((uint16_t)launch_torque_threshold						!= readFromFlash(18))	|| //LAUNCH_ASSIST_THRESHOLD
												((uint16_t)function_seatbelt_alarm_enabled				!= readFromFlash(19))	|| //SEATBELT_ALARM_DISABLED
												((uint16_t)function_pedal_booster_enabled				!= readFromFlash(20))	|| //PEDAL_BOOSTER_ENABLED
												((uint16_t)function_disable_odometer_blink				!= readFromFlash(21))	|| //DISABLE_ODOMETER_BLINK
												((uint16_t)function_show_race_mask						!= readFromFlash(22))	|| //SHOW_RACE_MASK
												((uint16_t)function_park_mirror							!= readFromFlash(23))	|| //PARK_MIRROR
												((uint16_t)function_acc_autostart						!= readFromFlash(24))	|| //ACC_AUTOSTART
												((uint16_t)function_close_windows_with_door_lock		!= readFromFlash(25))	|| //CLOSE_WINDOWS
												((uint16_t)function_open_windows_with_door_lock			!= readFromFlash(26))	){ //OPEN_WINDOWS
													//save it on flash
													saveOnflash();
											}
											dashboard_menu_indent_level=0;
											break;
										case 1: //{'[',' ',']','S','t','a','r','t','&','S','t','o','p'},
											function_smart_disable_start_stop_enabled=!function_smart_disable_start_stop_enabled;
											requestToDisableStartAndStop=0;
											break;
										case 2: //{'L','a','u','n','c','h','T','o','r','q','u','e',' ','1','0','0','N,'m',},
											launch_torque_threshold=launch_torque_threshold+25;
											if(launch_torque_threshold>600) launch_torque_threshold=25;
											break;
										case 3: //{'[',' ',']','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',},
											function_led_strip_controller_enabled = !function_led_strip_controller_enabled;
											break;
										case 4: //{'[',' ',']','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
											function_shift_indicator_enabled=!function_shift_indicator_enabled;
											break;
										case 5: //{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',},
											shift_threshold=shift_threshold+250;
											if(shift_threshold>6000) shift_threshold=1500;
											break;
										case 6: //{'[',' ',']','M','y','2','3',' ','I','P','C', },
											function_ipc_my23_is_installed=!function_ipc_my23_is_installed;
											break;
										case 7: //{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
											function_regeneration_alert_enabled=!function_regeneration_alert_enabled;
											//just for debug
											//uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
											//addToUARTSendQueue(tmpArr3, 1);

											break;
										case 8: //{'O',' ',' ','S','e','a','t','b','e','l','t',' ','A','l','a','r','m',' '},
											function_seatbelt_alarm_enabled=!function_seatbelt_alarm_enabled;
											break;
										case 9: //{'[',' ',']','R','o','u','t','e',' ','M','e','s','s','a','g','e','s', },
											function_route_msg_enabled=!function_route_msg_enabled;
											break;
										case 10: //{'[',' ',']','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',},
											function_esc_tc_customizator_enabled = !function_esc_tc_customizator_enabled;
											break;
										case 11: //{'[',' ',']','D','y','n','o',},
											function_dyno_mode_master_enabled=!function_dyno_mode_master_enabled;
											break;
										case 12: //{'[',' ',']','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
											function_acc_virtual_pad_enabled=!function_acc_virtual_pad_enabled;
											break;
										case 13: //{'[',' ',']','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
											function_front_brake_forcer_master=!function_front_brake_forcer_master;
											break;
										case 14: //{'[',' ',']','4','W','D',' ','D','i','s','a','b','l','e','r',},
											function_4wd_disabler_enabled=!function_4wd_disabler_enabled;
											break;
										case 15: //{'[',' ',']','C','l','e','a','r',' ','F','a','u','l','t','s',},
											function_clear_faults_enabled=!function_clear_faults_enabled;
											break;
										case 16: //{'[',' ',']','R','e','a','d',' ',' ','F','a','u','l','t','s',},
											function_read_faults_enabled=!function_read_faults_enabled;
											break;
										case 17: //{'[',' ',']','R','e','m','o','t','e',' ','S','t','a','r','t',},
											function_remote_start_Enabled=!function_remote_start_Enabled;
											break;
										case 18: //{'Ø',' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
											function_is_diesel_enabled=!function_is_diesel_enabled;
											break;
										case 19: //{'Ø',' ',' ','P','e','d','a','l',' ','B','o','o','s','t','e','r',' ',' '},
											function_pedal_booster_enabled++;
											if (function_pedal_booster_enabled>6) function_pedal_booster_enabled=0; //rotative selection 0=disabled, 1=auto, 2=Bypass, 3=All Weather map, 4=Natural Map, 5=Dynamic Map, 6=Race Map
											//Now let's inform the C2 and BH Baccable
											uint8_t tmpArr1[3]={C2_Bh_BusID,C2_Bh_cmdSetPedalBoostStatus,function_pedal_booster_enabled};
											addToUARTSendQueue(tmpArr1, 3);
											break;
										case 20: // {'O',' ',' ','O','d','o','m','e','t','e','r',' ','B','l','i','n','k',' '},
											function_disable_odometer_blink=!function_disable_odometer_blink;
											//Now let's inform the BH Baccable
											uint8_t tmpArr2[2]={BhBusID,BHcmdOdometerBlinkDefault};
											if(function_disable_odometer_blink) tmpArr2[1]=BHcmdOdometerBlinkDisable;
											addToUARTSendQueue(tmpArr2, 2);
											break;
										case 21: // {'O',' ',' ','S','h','o','w',' ','R','a','c','e',' ','M','a','s','k',' '},
											function_show_race_mask=!function_show_race_mask;
											//Now let's inform the C2 Baccable
											uint8_t tmpArr3[2]={C2BusID,C2cmdRaceMaskDefault};
											if(function_show_race_mask) tmpArr3[1]=C2cmdShowRaceMask;
											addToUARTSendQueue(tmpArr3, 2);
											break;
										case 22: // {'O',' ',' ','P','a','r','k',' ','M','i','r','r','o','r',' ',' ',' ',' '},
											function_park_mirror=!function_park_mirror;
											//Now let's inform the BH Baccable
											uint8_t tmpArr4[2]={BhBusID,BHcmdFunctParkMirrorDisabled};
											if(function_park_mirror) tmpArr4[1]=BHcmdFunctParkMirrorStoreCurPos;
											addToUARTSendQueue(tmpArr4, 2);
											break;
										case 23: //{'O',' ',' ','A','C','C','+',' ','A','u','t','o','s','t','a','r','t',' '},
											function_acc_autostart=!function_acc_autostart;
											break;
										case 24: //{'O',' ',' ','C','l','o','s','e',' ','W','i','n','d','o','w','s',' ',' '},
											function_close_windows_with_door_lock++;
											if(function_close_windows_with_door_lock>2) function_close_windows_with_door_lock=0;
											closeWindowsRequest=0;
											doorLocksRequestsCounter=0;
											//openWindowsRequest=0;
											//doorUnlocksRequestsCounter=0;
											break;
										case 25: //{'O',' ',' ','O','p','e','n',' ',' ','W','i','n','d','o','w','s',' ',' '},
											function_open_windows_with_door_lock++;
											if(function_open_windows_with_door_lock>2) function_open_windows_with_door_lock=0;
											openWindowsRequest=0;
											doorUnlocksRequestsCounter=0;
											break;
										default:
											break;
									}

									sendSetupDashboardPageToSlaveBaccable();
									break;
								case 10: //PARAMS SETUP MENU
									switch(params_setup_dashboardPageIndex){
										case 0: //{'S','A','V','E','&','E','X','I','T',},
											saveShownParamsOnflash();
											dashboard_menu_indent_level=0;
											break;
										default: //toggle hidden variable of current param
											shownParamsArray[params_setup_dashboardPageIndex-1]=!shownParamsArray[params_setup_dashboardPageIndex-1];
											break;
									}
									break;
								default:
									//we want to return main menu
									dashboard_menu_indent_level=0;
									sendMainDashboardPageToSlaveBaccable(); //print menu
							}
						}



					}
					wheelPressedButtonID=0x10; //button released
					lastPressedWheelButtonDuration=0;
					lastPressedSpeedUpWheelButtonDuration=0;
					break;
				case 0x90: //RES button was pressed
				case 0x50: //distance selector, used like RES, to manage the menu
					#ifndef HIDE_DASHBOARD_MENU
						lastPressedWheelButtonDuration++;

						if (wheelPressedButtonID==0x10 && wheelPressedButtonID!=0x90){//we pressed RES for at least one instant
							wheelPressedButtonID=0x89; //avoid returning here until button is not released
						}
						if (wheelPressedButtonID==0x89 && (lastPressedWheelButtonDuration>50)){//we pressed RES for around 2 seconds, therefore we want to enable/disable Baccable menu on dashboard
							wheelPressedButtonID=0x90; //avoid returning here until button is not released

							baccableDashboardMenuVisible=!baccableDashboardMenuVisible; //toggle visualizazion of the menu

							LOG("Dashboard vis: %d\r\n", baccableDashboardMenuVisible);

							if(!baccableDashboardMenuVisible){ //if menu needs to be hidden, print spaces to clear the string on dashboard
								clearDashboardBaccableMenu();
							}else{
								//dashboardPageIndex=0; //reset the page, just to be sure to show initial Baccable print
								//main_dashboardPageIndex=0; //shows initial baccable version
								//dashboard_menu_indent_level=0;
							}


						}
					#endif

					break;
				case 0x12: //Cruise Control Disabled/Enabled
					break;
				default:
			}
		}

		#ifndef PERMANENTLY_DISABLE_IMMO
			if(cruiseControlDisabled && ACC_Disabled ){ //if we are allowed to use the buttons of the cruise control
				if (currentRpmSpeed>400){ //if motor is on
					if(currentGear==0){ //gear is neutral
						if((rx_msg_data[0]==0x08) && ((wheelPressedButtonID==0x10) || (wheelPressedButtonID==0x08))){ //user is pressing CC soft speed up button and it was previously released (or pressed by baccable menu up here)
							lastPressedSpeedUpWheelButtonDuration++;
							if(lastPressedSpeedUpWheelButtonDuration>1267){ //around 30 seconds
								//avoid to return here
								wheelPressedButtonID=0xF8; //invent a new status to differentiate it from 0x08 used in baccable menu few lines of code up here
								lastPressedSpeedUpWheelButtonDuration=0; //unuseful here since it is done when button is released. just to be superstitious :-D.
								immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status
								floodTheBus=0; //ensure to reset this even if probably it is not needed
								if(saveOnflash((uint16_t)immobilizerEnabled)>253){ //if we get error while permanently storeing the parameter on flash
									immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status to the original status and avoid to report the user anything
									onboardLed_red_on(); //a problem occurred
								}else{
									onboardLed_blue_on(); //everything goes fine. change saved on flash
									if(immobilizerEnabled){ //if immo enabled
										executeDashboardBlinks=6; //blinks the dashboard brightness 3 times
									}else{
										executeDashboardBlinks=12; //blinks the dashboard brightness 6 times
									}
								}


							}
						}
						if(rx_msg_data[0]==0x10){ //user released the button
							lastPressedSpeedUpWheelButtonDuration=0;
							wheelPressedButtonID=0x10; //button released
						}
					}

				}
			}
		#endif
	#endif

}
