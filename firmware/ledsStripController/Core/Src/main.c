// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"

#if defined(ACT_AS_CANABLE)

	#include "usb_device.h"
	#include "usbd_cdc_if.h"
	#include "string.h"
#endif




#if defined(IMMOBILIZER_ENABLED)
	uint8_t immobilizerEnabled=1; //used to set filter in can.c
#else
	uint8_t immobilizerEnabled=0;
#endif


char *FW_VERSION="BACCABLE      V.2.0";  //this is used to store FW version, also shown on usb when used as slcan
float scaledVolume;
uint8_t scaledColorSet;

uint32_t debugTimer0;
uint32_t timeSinceLastReceivedAcceleratorMessage=0;

uint8_t ledsStripIsOn=0; //indicates if leds strip is on
uint8_t panicAlarmActivated=0; //indicates if the panic alarm was activated during last 10 minutes

//the following 2 arrays declares: RFHUB reset (first message) and panic alarm messages definition (the others)
CAN_TxHeaderTypeDef panicAlarmStartMsgHeader[5]={ 	{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAC7F1, .DLC=3},
													{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x1E340041, .DLC=4},
													{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
													{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
												};
uint8_t panicAlarmStartMsgData[5][8]={	{0x02,0x11,0x01,0x00,},
										{0x88,0x20,0x15,0x00,},
										{0x42,0x02,0xE2,0x00,0x00,0x00,0x01,0x56},
										{0x00,0x00,0xE2,0x00,0x00,0x00,0x00,0x00},
									 };

uint8_t startAndStopEnabled=1;
uint32_t lastTimeStartAndstopDisablerButtonPressed=0;

CAN_TxHeaderTypeDef shift_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2ED, .DLC=8}; //used when SHIFT_INDICATOR_ENABLED is defined
uint8_t shift_msg_data[8]; //used when SHIFT_INDICATOR_ENABLED is defined
uint32_t currentRpmSpeed=0;//used when SHIFT_INDICATOR_ENABLED is defined

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0};  //msg data
uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

//the following array stores buttons pressed password sequence
//these are possible values 0x90=RES,
//							0x12=Cruise control on/off,
//                          0x08=Cruise control speed gently up,
//                          0x00=Cruise control speed strong up,
//                          0x18=Cruise control speed gently down,
//                          0x20=Cruise control speed strong down
// WARNING: when you press cruise control strong up, before and after it, also cruise control gently up
//          message is fired, therefore your sequence will be altered. A workaround
//          is to use both messages in the sequence, or to use just the gently up/down message.

//uint32_t buttonPressedTimeArray[20] = {0};  //0=RES, 1=Cruise control on/off, 2=Cruise control speed gently up, 3=Cruise control speed strong up, 4=Cruise control speed gently down, 5=Cruise control speed strong down
uint8_t ButtonPressSequence1Index=0;
uint8_t ButtonPressSequence1Len=5;
uint8_t ButtonPressSequence1[5]={0x90,0x08,0x00,0x08,0x90}; //current password: RES - CC gently up  - CC strong up - CC gently up - RES

//future growth
uint8_t ButtonPressSequence2Index=0;
uint8_t ButtonPressSequence2Len=8;
uint8_t ButtonPressSequence2[8]={0x90,0x08,0x00,0x08,0x18,0x20,0x18,0x12}; //current password: RES - CC gently up - CC strong up - CC gently up - CC gently down - CC strong down - CC gently down - CC on/off

uint8_t  lastPressedWheelButton=0xff; //default value, means no button pressed on the wheel
uint32_t lastPressedWheelButtonTime=0;//stores the last time a wheel button was pressed, in msec from boot
uint32_t lastPressedWheelButtonDuration=0x00; //default value

uint8_t floodTheBus=0;
uint32_t floodTheBusStartTime=0;
uint32_t floodTheBusLastTimeSent=0;
extern can_txbuf_t txqueue;

uint8_t currentDNAmode; //used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
uint8_t DNA_msg_data[8];//used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
CAN_TxHeaderTypeDef DNA_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8}; //used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
uint32_t LANEbuttonPressLastTimeSeen=0; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
uint8_t LANEbuttonPressCount=0; //stores number of times this message field was received
uint8_t ESCandTCinversion=0; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)
uint32_t lastSentTelematic_display_info_msg_Time=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
uint8_t telematic_display_info_field_totalFrameNumber=5; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
uint8_t telematic_display_info_field_frameNumber=0; //current frame //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
uint8_t telematic_display_info_field_infoCode=0x0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
uint8_t paramsStringArray[10][18]={ {'B', 'A', 'C', 'C', 'A', 'B', 'L', 'E', ' ', 'V', '.', '2', '.', '0', },
									{'P', 'a', 'g', 'e', ' ', '1', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '2', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '3', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '4', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '5', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '6', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '7', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '8', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', },
									{'P', 'a', 'g', 'e', ' ', '9', ' ', 'i', 's', ' ', 'E', 'm', 'p', 't', 'y', }};//string array to show on dashboard - Second dimension shall be 3 x (telematic_display_info_field_totalFrameNumber +1 ). Example: 3x(11+1)=36  //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.

uint8_t paramsStringArrayIndex=0; //indice del messaggio da spedire - potremo cambiare indice con i pulsanti del cruise control) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
uint8_t paramsStringCharIndex=0; //indice del prossimo carattere della stringa corrente da spedire //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8}; //used when SHOW_PARAMS_ON_DASHBOARD is defined
uint8_t telematic_display_info_msg_data[8]; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
uint8_t sentMsgCounter=0;//--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
int8_t SHOW_PARAMS_Command=0; //user command to show another parameter //--// 1=next, -1=previous, 0=no command  //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality

int main(void){
	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	can_init(); //initialize can interface
	//onboardLed_red_on(); This line doesn't work cause hardware is still initiating


	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ESC_TC_CUSTOMIZATOR_ENABLED))  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(SHOW_PARAMS_ON_DASHBOARD))
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_125K);//set can speed to 125kpbs
		can_enable(); //enable can port
	#endif

	while (1){
		debugTimer0=HAL_GetTick();
		onboardLed_process();
		can_process();
		#if defined(ACT_AS_CANABLE)
			cdc_process(); //processa dati usb
			//just for test
			//char *data = "Hello World from USB CDC\n";
			//CDC_Transmit_FS((uint8_t*)data, strlen(data));
			//HAL_Delay (1000);

			//just for test, we can periodically send a packet to can bus
			//we can set the can bus in loopback mode, to receive back each sent message.
			// To set loopback mode, in function can_enable (can.c) we shall set can_handle.Init.Mode = CAN_MODE_LOOPBACK
			// This way we will receive whatever we send
			// Before to send can messages, speed shall be set and can port shall be enabled,
			//can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
			//can_enable(); //enable can port
			//Then we can prepare and send the following test packet
			//CAN_TxHeaderTypeDef testMsgHeader;
			//testMsgHeader.IDE= CAN_ID_STD;
			//testMsgHeader.RTR = CAN_RTR_DATA;
			//testMsgHeader.StdId=0x0412;
			//testMsgHeader.DLC=5;
			//uint8_t testMsgData[8] = {0};
			//testMsgData[0]=0x01;
			//testMsgData[1]=0x01;
			//testMsgData[2]=0x01;
			//testMsgData[3]=0x01;
			//testMsgData[4]=0xE6; //pedal position
			//can_tx(&testMsgHeader, testMsgData);
		#endif

		#if defined(LED_STRIP_CONTROLLER_ENABLED)
			//don't act as canable. One USB port pin is used to control leds.
			vuMeterInit(); //initialize leds strip controller - this is called many times to divide the operations on more loops
			if(ledsStripIsOn){ //if the strip is on,
				if(timeSinceLastReceivedAcceleratorMessage+10000<HAL_GetTick() ){ //if no can interesting message for 10 seconds,  shutdown the leds to save energy
					shutdownLedsStrip();
					ledsStripIsOn=0; //entriamo solo una volta
				}
			}

		#endif

		if(immobilizerEnabled){
			//the following it is used only by IMMOBILIZER functionality
			if(floodTheBus){ //WHEN THIS IS ACTIVATED, THE THIEF WILL NOT BE ABLE TO CONNECT TO RFHUB, AND CAR WILL NOT SWITCH ON.
				if(floodTheBusLastTimeSent+10<HAL_GetTick()){
					can_tx(&panicAlarmStartMsgHeader[0], panicAlarmStartMsgData[0]); //sends the message on can bus that resets the connection to RFHUB
					floodTheBusLastTimeSent=HAL_GetTick();
				}

				if(!panicAlarmActivated){ //if panic alarm is not activated, we shall activate it after 1 second (1 second to avoid stop and start simultaneous)
					if(floodTheBusStartTime+1000<HAL_GetTick()){
						for (uint8_t i=0;i<15;i++){
							can_tx(&panicAlarmStartMsgHeader[1], panicAlarmStartMsgData[1]);
						}
						can_tx(&panicAlarmStartMsgHeader[2], panicAlarmStartMsgData[2]);
						panicAlarmActivated=1;
					}
				}
				if(floodTheBusStartTime+10000 < HAL_GetTick()){ //if the bus is flooded since 10 seconds, stop flooding it
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

		#if defined(DISABLE_START_STOP)
			if(startAndStopEnabled){
				if(HAL_GetTick()>60000){ //after one minute the car should be on
					if(lastTimeStartAndstopDisablerButtonPressed==0){ //first time we arrive here, go inside
						// We will now short gpio to ground in order to disable start&stop car functionality. This will simulate car start&stop button press
						HAL_GPIO_WritePin(START_STOP_DISABLER, 0); // I use swclk pin, pin37, PA14
						lastTimeStartAndstopDisablerButtonPressed=HAL_GetTick();
						onboardLed_red_on();
					}

					if(lastTimeStartAndstopDisablerButtonPressed+500<HAL_GetTick()){ //if pressed since 500msec
						HAL_GPIO_WritePin(START_STOP_DISABLER, 1); //return to 1
						startAndStopEnabled=0;
						onboardLed_blue_on();

					}
				}
			}
		#endif

		#if defined(SHOW_PARAMS_ON_DASHBOARD)
			//send one msg to write something on the dashboard each 50msec (one frame each 550msec)
			if (lastSentTelematic_display_info_msg_Time+200<HAL_GetTick()){
				lastSentTelematic_display_info_msg_Time=HAL_GetTick();


				if(telematic_display_info_field_frameNumber==0){
					//IF THERE IS A USER REQUEST TO SHOW ANOTHER PARAMETER PAGE, DO IT
					if(SHOW_PARAMS_Command==-1 || SHOW_PARAMS_Command==1){ //-1=previous 1=next
						SHOW_PARAMS_Command=0; //no command to execute
						//clear screen
						telematic_display_info_msg_data[0]= 0;
						telematic_display_info_msg_data[1]=0x11;
						telematic_display_info_msg_data[2]=0;
						telematic_display_info_msg_data[3]= 0x20;
						telematic_display_info_msg_data[4]=0;
						telematic_display_info_msg_data[5]=0;
						telematic_display_info_msg_data[6]=0;
						telematic_display_info_msg_data[7]=0;
						//send it
						can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet

						//set index
						paramsStringArrayIndex += SHOW_PARAMS_Command;
						// make a rotative menu
						if(paramsStringArrayIndex==255) paramsStringArrayIndex=9;
						if(paramsStringArrayIndex==10)  paramsStringArrayIndex=0;
					}

					//update values TO SHOW each time we are in frame 0, by means of previously extracted parameters
					//we should update values here, and fix names in array declaration
					//but now I don't know which parameters to print
					switch(paramsStringArrayIndex){
						case 1:
							paramsStringArray[paramsStringArrayIndex][0]='P';
							paramsStringArray[paramsStringArrayIndex][1]='a';
							paramsStringArray[paramsStringArrayIndex][2]='r' ;
							paramsStringArray[paramsStringArrayIndex][3]='1';
							paramsStringArray[paramsStringArrayIndex][4]=':';
							paramsStringArray[paramsStringArrayIndex][5]='0';
							paramsStringArray[paramsStringArrayIndex][6]='0';
							paramsStringArray[paramsStringArrayIndex][7]='1';
							paramsStringArray[paramsStringArrayIndex][8]='%';
							paramsStringArray[paramsStringArrayIndex][9]='|';
							paramsStringArray[paramsStringArrayIndex][10]='P';
							paramsStringArray[paramsStringArrayIndex][11]='a';
							paramsStringArray[paramsStringArrayIndex][12]='r';
							paramsStringArray[paramsStringArrayIndex][13]='2';
							paramsStringArray[paramsStringArrayIndex][14]=':';
							paramsStringArray[paramsStringArrayIndex][15]='0';
							paramsStringArray[paramsStringArrayIndex][16]='0';
							paramsStringArray[paramsStringArrayIndex][17]='2';
							break;
						case 2:
							paramsStringArray[paramsStringArrayIndex][0]='P';
							paramsStringArray[paramsStringArrayIndex][1]='a';
							paramsStringArray[paramsStringArrayIndex][2]='r' ;
							paramsStringArray[paramsStringArrayIndex][3]='3';
							paramsStringArray[paramsStringArrayIndex][4]=':';
							paramsStringArray[paramsStringArrayIndex][5]='0';
							paramsStringArray[paramsStringArrayIndex][6]='0';
							paramsStringArray[paramsStringArrayIndex][7]='2';
							paramsStringArray[paramsStringArrayIndex][8]='%';
							paramsStringArray[paramsStringArrayIndex][9]='|';
							paramsStringArray[paramsStringArrayIndex][10]='P';
							paramsStringArray[paramsStringArrayIndex][11]='a';
							paramsStringArray[paramsStringArrayIndex][12]='r';
							paramsStringArray[paramsStringArrayIndex][13]='4';
							paramsStringArray[paramsStringArrayIndex][14]=':';
							paramsStringArray[paramsStringArrayIndex][15]='0';
							paramsStringArray[paramsStringArrayIndex][16]='0';
							paramsStringArray[paramsStringArrayIndex][17]='3';
							break;
						case 3:
							paramsStringArray[paramsStringArrayIndex][0]='P';
							paramsStringArray[paramsStringArrayIndex][1]='a';
							paramsStringArray[paramsStringArrayIndex][2]='r' ;
							paramsStringArray[paramsStringArrayIndex][3]='5';
							paramsStringArray[paramsStringArrayIndex][4]=':';
							paramsStringArray[paramsStringArrayIndex][5]='0';
							paramsStringArray[paramsStringArrayIndex][6]='0';
							paramsStringArray[paramsStringArrayIndex][7]='3';
							paramsStringArray[paramsStringArrayIndex][8]='%';
							paramsStringArray[paramsStringArrayIndex][9]='|';
							paramsStringArray[paramsStringArrayIndex][10]='P';
							paramsStringArray[paramsStringArrayIndex][11]='a';
							paramsStringArray[paramsStringArrayIndex][12]='r';
							paramsStringArray[paramsStringArrayIndex][13]='6';
							paramsStringArray[paramsStringArrayIndex][14]=':';
							paramsStringArray[paramsStringArrayIndex][15]='0';
							paramsStringArray[paramsStringArrayIndex][16]='0';
							paramsStringArray[paramsStringArrayIndex][17]='4';
							break;
						default:
					}
				}

				//prepare msg to send

				//total frame number is on byte 0 from bit 7 to 3
				telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0xF8) | ((telematic_display_info_field_totalFrameNumber<<3) & 0xF8);

				//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
				telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0x07) | ((telematic_display_info_field_frameNumber>>2) & 0x07);
				telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0xC0) | ((telematic_display_info_field_frameNumber<<6) & 0xC0);

				//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
				telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0x3F) | ((telematic_display_info_field_infoCode) & 0x3F);

				//UTF text 1 is on byte 2 and byte 3
				telematic_display_info_msg_data[2]=0;
				telematic_display_info_msg_data[3]=paramsStringArray[paramsStringArrayIndex][paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char

				//UTF text 2 is on byte 4 and byte 5
				telematic_display_info_msg_data[4]=0;
				telematic_display_info_msg_data[5]=paramsStringArray[paramsStringArrayIndex][paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char

				//UTF text 3 is on byte 6 and byte 7
				telematic_display_info_msg_data[6]=0;
				telematic_display_info_msg_data[7]=paramsStringArray[paramsStringArrayIndex][paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char

				//send it
				can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet
				onboardLed_blue_on();

				if( paramsStringCharIndex==(telematic_display_info_field_totalFrameNumber+1) * 3) { //if we sent the entire string
					paramsStringCharIndex=0; //prepare to send first char of the string

				}
				telematic_display_info_field_frameNumber++; //prepare for next frame to send
				if (telematic_display_info_field_frameNumber>telematic_display_info_field_totalFrameNumber){//if we sent all the frames
					telematic_display_info_field_frameNumber=0; //prepare to send first frame
				}
			}
		#endif

		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#endif

				if (rx_msg_header.RTR == CAN_RTR_DATA){
					switch(rx_msg_header.IDE){
						case CAN_ID_EXT:
							if(immobilizerEnabled){
								//if it is a message of connection to RFHUB, reset the connection periodically, but start the panic alarm only once
								if(floodTheBus==0){ //if we are not flooding the bus
									if ((rx_msg_header.ExtId==0x18DAC7F1) ||( (rx_msg_header.ExtId==0x18DAF1C7) && (floodTheBusStartTime==0)) ){  // If msg from thief or (reply from rfhub && it is the first time that it occurs)
										//thief connected to RFHUB: we shall reset the RFHUB and start the alarm
										//start to flood the bus with the rfhub disconnect message
										floodTheBus=1;
										floodTheBusStartTime=HAL_GetTick();
										onboardLed_blue_on();
									}
								}
							} //end of immobilizer section
							break;
						case CAN_ID_STD: //if standard ID

							switch(rx_msg_header.StdId){ //messages in this switch is on C can bus, only when on different bus, the comments explicitly tells if it is on another can bus

								case 0x00000090:
									//on BH can bus, slow bus at 125kbps, this message contains:

									//total frame number is on byte 0 from bit 7 to 3
									//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
									//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
									//UTF text 1 is on byte 2 and byte 3
									//UTF text 2 is on byte 4 and byte 5
									//UTF text 3 is on byte 6 and byte 7
									break;
								case 0x000000FC: //message to dashboard containing rpm speed and not only
									#if defined(SHIFT_INDICATOR_ENABLED)
										if(rx_msg_header.DLC==8){
											//extract rpm speed and set the variable currentRpmSpeed
											currentRpmSpeed=(rx_msg_data[0] *256 + (rx_msg_data[1] & ~0x3) )/4;
											//onboardLed_blue_on();
										}
									#endif
									//engine speed fail is on byte1 bit 1.
									//engine StopStart Status is on byte 1 bit 0 and byte 2 bit 7
									//engine Status is on byte 2 bit 6 and bit 5.
									//gas pedal position is on byte 2 from bit 4 to 0 and byte 3 from bit 7 to 5.
									//gas pedal position fail is on byte3 bit 4.
									//.....
									//alternator fail is on byte 3, bit1.
									//stopStart status is on byte3 bit 0 and byte4 bit7.
									//CC brake intervention request is on byte 4, bit5
									//bank deactivation status is on byte5, bit 7 and 6
									//CC brake intervention is on byte 5 from bit 5 to 0 and byte 6 from bit 7 to 4.
									break;
								case 0x000001F0:
									//clutch interlock is on byte 0 bit 7
									//clutch upstop is on byte0 bit 6
									//actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
									//analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.

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
								case 0x000002ED: //message to dashboard containing shift indicator
									#if defined(SHIFT_INDICATOR_ENABLED)
										if(rx_msg_header.DLC==8){
											if (currentRpmSpeed>SHIFT_THRESHOLD-1 ){ //if the rpm speed is above SHIFT_THRESHOLD rpm, then the packet need to be modified, therefore,
												//copy 8 bytes from msgdata rx (rx_msg_data) to msg data tx (shift_msg_data)
												memcpy(&shift_msg_data, &rx_msg_data, 8);

												if(currentRpmSpeed>(SHIFT_THRESHOLD-1) &&  currentRpmSpeed<(SHIFT_THRESHOLD+500)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) to 01 (binary) meaning "gear shift urgency level 1"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x1 & 0x3);
												}
												if(currentRpmSpeed>(SHIFT_THRESHOLD+500-1) &&  currentRpmSpeed<(SHIFT_THRESHOLD+1000)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) of the message to 10 (binary) meaning "gear shift urgency level 2"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x2 & 0x3);
												}else if(currentRpmSpeed>(SHIFT_THRESHOLD+1000-1)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) of the message to 11 (binary) meaning "gear shift urgency level 3"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x3 & 0x3);
												}
												can_tx(&shift_msg_header, shift_msg_data); //transmit the modified packet
												//onboardLed_red_on();
											}

										}
									#endif

										//EngineWaterTemperature is on byte0
										//fuel consumption is on byte 4 bit 0 to 3, byte 5, and byte 6 from bit 7 to 3
									break;
								case 0x000002EE: // presente solo su BH can bus a 125kbps
									//this message contains the following radio buttons on the steering wheel:
									//radio right button is on byte 3 bit6 (1=button pressed)
									//radio left button on the steering wheel is on byte 3 bit4 (1=button pressed)
									//radio Voice command button is on byte 3 bit2 (1= button pressed)
									//phone call button is on byte3 bit0(1=button pressed)
									//volume  is on byte 4 (volume up increases the value, volume down reduces the value. once arrived to 255 restarts from 0 and under 0 goes to 255)
									//volume change is on byte5 bit 7 and bit6 (1=volume was increased rotation, 2=volume decreased rotation, 3=volume mute button press) (then reading the entire byte we will see respectively, 0x40, 0x80,  0xC0)

								case 0x000002EF: //se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
									#if defined(LED_STRIP_CONTROLLER_ENABLED)
										if(rx_msg_header.DLC==8){
											scaledColorSet=scaleColorSet(rx_msg_data[0] & ~0xF); //prima di tutto azzeriamo i primi 4 bit meno significativi, poi scala il dato con la funzione scaleColorSet, per prepararlo per l'invio alla classe vumeter
											vuMeterUpdate(scaledVolume,scaledColorSet);
										}
									#endif

									//actual gear status is on byte 0 from bit 7 to 4 (0x0=neutral, 0x1 to 0x6=gear 1 to 6, 0x07=reverse gear, 0x8 to 0xA=gear 7 to 9, 0xF=SNA)
									//suggested gear status is on byte 0 from bit 3 to 0
									//DPF Regeneration mode is on byte 1 bit 7.
									//SAM info is on byte 1 from bit 3 to 0
									//stop start fault status is on byte 2 bit 7
									//..
									//boost pressure indication is on byte 3 bit from 6 to 0 and byte 4  bit 7


									break;
								case 0x000002FA: // Button is pressed on left area of the wheel
									// These Buttons are detected only if the main panel of the car is on.

									// We commented this section since we are not using it now. Uncomment to use it.
									// The cycle duration could increase and make blink the red led onboard.
									// The action of the button's sequence is left empty for you to add it.


									// it shall be different than 0x10 because sometimes is found on the bus,
									// but we are not interested in it.
									/*
									if( (rx_msg_header.DLC==3) && (rx_msg_data[0]!=0x10) ){
										//if 2 seconds has passed since last button press, restart password sequence
										if (lastPressedWheelButtonTime+2000<HAL_GetTick()){
											ButtonPressSequence1Index=0;
											lastPressedWheelButtonDuration=0;
											lastPressedWheelButtonTime=HAL_GetTick();
										}
										//if the previous button is still pressed, we shall consider it as the same button press
										if ((lastPressedWheelButton== rx_msg_data[0]) ){
											lastPressedWheelButtonDuration=lastPressedWheelButtonDuration+ (HAL_GetTick()-lastPressedWheelButtonTime) ;
										}else{
											//another button was pressed, let's consider it as a button release event:
											//  if the button was pressed for at least 100msec, and if it is the button expected by the secret sequence
											if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index])){
												onboardLed_blue_on();
												ButtonPressSequence1Index++; //prepare for next button in the sequence
											}else{
												//something else arrived. restart the sequence
												ButtonPressSequence1Index=0;
											}
											lastPressedWheelButtonDuration=0;
										}

										//manage the particular case of last button of the sequence pressed for enough time
										if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index]) && ButtonPressSequence1Index==ButtonPressSequence1Len-1){
											ButtonPressSequence1Index++;
										}

										lastPressedWheelButton= rx_msg_data[0]; //store last pressed button
										lastPressedWheelButtonTime=HAL_GetTick(); //store last time a button was pressed

										//if password sequence was fully correctly typed...
										if(ButtonPressSequence1Index==ButtonPressSequence1Len){
											// reset password sequence, so that we can type it again in the future,
											// then do what you shall do
											ButtonPressSequence1Index=0;
											onboardLed_red_on();
											// what do we want to do? this will work only if the bus is not flood
											// and the car panel is on.
											// future usage....
										}
									}
									*/
									break;
								case 0x00000384:
									#if defined(ESC_TC_CUSTOMIZATOR_ENABLED)
										//on C2 can bus, msg 0x384 contains, in byte3, bit6 contains left stalk button press status (LANE indicator button)
										if((rx_msg_data[3] & 0x40) ==0x40){ // left stalk button was pressed (lane following indicator)

											LANEbuttonPressLastTimeSeen=HAL_GetTick();//save current time it was pressed as LANEbuttonPressLastTimeSeen
											LANEbuttonPressCount++;
											if (LANEbuttonPressCount>8){ //8 is more or less 2 seconds
												ESCandTCinversion=!ESCandTCinversion; //toggle the status
												//onboardLed_red_on();
												LANEbuttonPressCount=0; //reset the count
											}
										}else{
											if(LANEbuttonPressLastTimeSeen+1000<HAL_GetTick()){ // if LANEbuttonPressLastTimeSeen, is older than 1 second ago, it means that button was released
												LANEbuttonPressCount=0;// reset the count assigning it zero
											}
										}

										if(currentDNAmode!=(rx_msg_data[1]& 0x7C)){ //RDNA mode was changed, reset the ESCandTCinversion
											ESCandTCinversion=0;
										}
										//current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08], 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
										currentDNAmode=rx_msg_data[1] & 0x7C; //7C is the mask from bit 6 to 2
										if (ESCandTCinversion){
											memcpy(&DNA_msg_data, &rx_msg_data, 8);
											if(currentDNAmode==0x30){ //race
												DNA_msg_data[1]= (DNA_msg_data[1] & ~0x7C) | (0x08 & 0x7C); //set Dynamic mode (0x08) to enable ESC and TC
											}else{
												DNA_msg_data[1] = (DNA_msg_data[1] & ~0x7C) | (0x30 & 0x7C);  //set Race mode (0x30) to disable ESC and TC
											}
											can_tx(&DNA_msg_header, DNA_msg_data); //transmit the modified packet
											//onboardLed_blue_on();
										}
									#endif

									//Command Ignition Status is on byte0 from bit 3 to 1.
									//Command Ignition Fail Status is on byte 0 bit0 and in byte1 bit7.
									//Drive Style Status (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
									//External temperature is on byte 1 from bit 1 to 0 and on byte 2 from bit 7 to bit 1.
									//External temperature fail is on byte2 bit0
									//Low Beam Status is on byte3 bit7
									//Lane Indicator button status (left stalk button) is on byte 3 bit6.
									//Power Mode Status is on byte 3 from bit 5 to 4.
									//Park Brake Status is on byte 3 bit 3.
									//Int. Relay Fail Status is on byte 4 from bit 7 to 6
									//SuspensionLevel is on byte 5 bit0 and byte 6 bit7.

									break;
								case 0x00000412: //se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il valore é >51 (sfrutto le info ottenute sniffando)
									#if defined(LED_STRIP_CONTROLLER_ENABLED)
										if( (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51) ){
											timeSinceLastReceivedAcceleratorMessage=HAL_GetTick();
											ledsStripIsOn=1;
											scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
											vuMeterUpdate(scaledVolume,scaledColorSet);
										}
									#endif
									break;
								case 0x000004B2:
									//engine oil level is in byte 0 from bit 7 to 3.
									//engine oil over fill status is on byte 0, bit 2.
									//engine oil min. is on byte 0 bit 1
									//engine oil pressure is on byte 0, bit 0 and on byte 1 from bit 7 to 1.
									//power mode status is on byte 1 bit 0 and on byte 2 bit 7.
									//engine water level is on byte 2 bit 6.
									//engine oil temperature is on byte 2 from bit 5 to 0 and on byte 3 from bit 7 to 6.
									//engine oil temperature warning light is on byte 3 bit 5.
									break;
								default:
							}

							break;
						default:
					}
				}
			}
		}

		//for debug, measure the loop duration
		if (HAL_GetTick()>debugTimer0+2){
			onboardLed_red_on();
		}
	}
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

//System Clock Configuration
void SystemClock_Config(void){
  HAL_Init();
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK){
    Error_Handler();
  }

  //the following part is used by usb, used by canable

  // Set USB clock source to HSI48 (48 MHz)
  RCC_PeriphCLKInitTypeDef PeriphClkInit= {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
	  Error_Handler();
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

void Error_Handler(void){
  __disable_irq();
  while (1){}
}

// Disable all interrupts
void system_irq_disable(void){
	__disable_irq();
	__DSB();
	__ISB();
}


// Enable all interrupts
void system_irq_enable(void){
        __enable_irq();
}

void system_hex32(char *out, uint32_t val){
	char *p = out + 8;
	*p-- = 0;
	while (p >= out) {
		uint8_t nybble = val & 0x0F;
		if (nybble < 10)
			*p = '0' + nybble;
		else
			*p = 'A' + nybble - 10;
		val >>= 4;
		p--;
	}
}

#ifdef  USE_FULL_ASSERT
	/**
	  * @brief  Reports the name of the source file and the source line number
	  *         where the assert_param error has occurred.
	  * @param  file: pointer to the source file name
	  * @param  line: assert_param error line source number
	  * @retval None
	  */
	void assert_failed(uint8_t *file, uint32_t line){
	  /* USER CODE BEGIN 6 */
	  /* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	  /* USER CODE END 6 */
	}
#endif /* USE_FULL_ASSERT */
