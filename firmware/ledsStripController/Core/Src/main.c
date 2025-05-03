// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"
#include "globalVariables.h"


<<<<<<< Updated upstream
#if defined(ACT_AS_CANABLE)
	//#include "usbd_def.h"
//	#include "usb_device.h"
	#include "usbd_cdc_if.h"
//	#include "string.h"
#endif





#if defined(C2baccable)
	//ESC_TC_CUSTOMIZATOR_ENABLED
	uint8_t currentDNAmode;
	uint8_t DNA_msg_data[8];
	CAN_TxHeaderTypeDef DNA_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8};
	uint8_t ESCandTCinversion=0; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)
	uint32_t LANEbuttonPressLastTimeSeen=0; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
	uint8_t LANEbuttonPressCount=0; //stores number of times this message field was received

	//DYNO_MODE
	uint8_t DynoModeEnabled=0;
	uint8_t DynoStateMachine=0xff; //State machine for dyno messages sequence. frm 00 to 03 = dyno message sequence is beeing transmitted. FF= inactive
	uint16_t testerMsgSent=0;
	uint8_t DYNO_msg_data[5][6]={{0x02,0x10,0x03,},{0x03,0x22,0x30,0x02,},{0x05,0x2E,0x30,0x02,0x00,0x01},{0x05,0x2E,0x30,0x02,0xFF,0x01}}; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno
	CAN_TxHeaderTypeDef DYNO_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=6};

	uint32_t DynoStateMachineLastUpdateTime=0; //stores time (in milliseconds from power on) when Park Assist button press was read last time
	uint8_t ParkAssistButtonPressCount=0; //stores number of times this message field was received

	//FRONT_BRAKE_FORCER
	uint32_t last_sent_rear_brake_msg_time=0;

	CAN_TxHeaderTypeDef rearBrakeMsgHeader[4]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=5},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=8},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3}};
	uint8_t rearBrakeMsgData[4][8]= {{0x04, 0x2F, 0x5A, 0xBD, 0x00,},{0x07, 0x2F, 0x5A, 0xBD, 0x03, 0x27, 0x10, 0x03},{0x02, 0x3E, 0x80,},{0x02, 0x10, 0x40,}}; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(disable front brakes) (periodic)

#endif

#if defined(BHbaccable)
	uint32_t lastSentTelematic_display_info_msg_Time=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	uint8_t telematic_display_info_field_totalFrameNumber=(DASHBOARD_MESSAGE_MAX_LENGTH / 3) - 1; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_frameNumber=0; //current frame //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_infoCode=0x09; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t paramsStringCharIndex=0; // next char to send index - Used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8}; //used when SHOW_PARAMS_ON_DASHBOARD is defined
	uint8_t telematic_display_info_msg_data[8]; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t requestToSendOneFrame=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard

	//Message to generate sound indication (chime)
	uint8_t CHIME_msg_data[8];
	CAN_TxHeaderTypeDef CHIME_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5AC, .DLC=8};
	uint8_t requestToPlayChime=0;

#endif

//CLEAR_FAULTS_ENABLED
uint8_t function_clear_faults_enabled=1; //default enabled. saved on flash
uint8_t clearFaultsRequest=0; //if enabled, sends messages to clear faults
uint32_t last_sent_clear_faults_msg=0;
uint8_t clearFaults_msg_data[5]={0x04,0x14,0xff,0xff,0xff}; //message to clear DTC
CAN_TxHeaderTypeDef clearFaults_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA00F1, .DLC=5};

//
uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]={' ',}; //used if SHOW_PARAMS_ON_DASHBOARD or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is declared - it contains string to print on dashboard

float currentSpeed_km_h=0; //current vehicle speed

uint32_t weCanSendAMessageReply=0; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
uint32_t debugTimer0;

UART_HandleTypeDef huart2; // this is the serial line between baccables -- used with SHOW_PARAMS_ON_DASHBOARD and SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE and low consumption define functionalities

uint32_t currentRpmSpeed=0;	//used by C1baccable
uint8_t currentGear=0; 		//used by C1baccable

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0,};  //msg data

uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

uint8_t _4wd_disabled=0; //if =4 disables 4wd
uint8_t front_brake_forced=0; //if=5 disables Front brakes
uint8_t commandsMenuEnabled=1; //if 0 disables the up-down buttons to change menu position

//the following array stores buttons pressed password sequence (future growth now commented)
//these are possible values 0x90=RES,
//							0x11=Adaptive Cruise control on/off
//							0x12=Cruise control on/off,
//                          0x08=Cruise control speed gently up,
//                          0x00=Cruise control speed strong up,
//                          0x18=Cruise control speed gently down,
//                          0x20=Cruise control speed strong down
// WARNING: when you press cruise control strong up, before and after it, also cruise control gently up
//          message is fired, therefore your sequence will be altered. A workaround
//          is to use both messages in the sequence, or to use just the gently up/down message.

//uint32_t buttonPressedTimeArray[20] = {0};  //0=RES, 1=Cruise control on/off, 2=Cruise control speed gently up, 3=Cruise control speed strong up, 4=Cruise control speed gently down, 5=Cruise control speed strong down
//uint8_t ButtonPressSequence1Index=0;
//uint8_t ButtonPressSequence1Len=5;
//uint8_t ButtonPressSequence1[5]={0x90,0x08,0x00,0x08,0x90}; //current password: RES - CC gently up  - CC strong up - CC gently up - RES

//uint8_t ButtonPressSequence2Index=0;
//uint8_t ButtonPressSequence2Len=8;
//uint8_t ButtonPressSequence2[8]={0x90,0x08,0x00,0x08,0x18,0x20,0x18,0x12}; //current password: RES - CC gently up - CC strong up - CC gently up - CC gently down - CC strong down - CC gently down - CC on/off

=======
>>>>>>> Stashed changes
int main(void){
	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	can_init(); //initialize can interface
	//onboardLed_red_on(); This line doesn't work cause hardware is still initiating
	#if defined(C1baccable)
		C1baccableInitCheck();
	#endif

	uart_init();

	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (defined(C1baccable) || defined(C2baccable) )  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(BHbaccable))
		BHbaccableInitCheck();
	#endif

	while (1){

		currentTime=HAL_GetTick();
		onboardLed_process();
		can_process();
		processUART();


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

		#if defined(C1baccable)
			C1baccablePeriodicCheck();
		#endif

		#if defined(C2baccable)
			C2PeriodicCheck();
		#endif

		#if defined(BHbaccable) //this is the baccable slave
			BHperiodicCheck();
		#endif

		#if (defined(C1baccable) || defined(C2baccable) || defined(BHbaccable))
			if(clearFaultsRequest>0){
				//clear faults if requested
				if(currentTime-last_sent_clear_faults_msg>25){
					last_sent_clear_faults_msg= currentTime;

					#if defined(C1baccable)
						if(clearFaultsRequest==255){
							//ask to Baccable on C2 and bH bus, to reset faults //to be done
							uint8_t tmpArr[1]={AllResetFaults};
							addToUARTSendQueue(tmpArr, 1);
						}
					#endif
					//send a reset request
					clearFaults_msg_header.ExtId= 0x18DA00F1 | ((uint32_t)clearFaultsRequest<<8);
					can_tx(&clearFaults_msg_header, clearFaults_msg_data); //transmit the request

					clearFaultsRequest--;
				}
			}
		#endif

		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					onboardLed_blue_on();
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#endif

				#if defined(C1baccable)
					lastReceivedCanMsgTime=currentTime;
				#endif

				if (rx_msg_header.RTR == CAN_RTR_DATA){
					switch(rx_msg_header.IDE){
						case CAN_ID_EXT:
							processingExtendedMessage();
							break;
						case CAN_ID_STD: //if standard ID
							processingStandardMessage();
							break;
						default:
					}
				}
			}
		}

		//for debug, measure the loop duration
		if (HAL_GetTick()-currentTime>2){
			onboardLed_red_on();
		}
	}
}








