/*
 * globalVariables.c
 *
 *  Created on: Apr 27, 2025
 *      Author: GauchoHP
 */
#include "globalVariables.h"


const char *FW_VERSION=_FW_VERSION;

// force print
#pragma message ("FW_VERSION: " _FW_VERSION)

#if defined(UCAN_BOARD_LED_INVERSION)
	const uint8_t led_light_on_bit=1;
#else
	const uint8_t led_light_on_bit=0;
#endif



#if defined(C1baccable)
	float chronometerElapsedTime_0_100_km_h=60; //stores time statistic in seconds
	float chronometerElapsedTime_100_200_km_h=60; //stores time statistic in seconds
	uint8_t statistics_0_100_started=0; //stores id the statistic timer has started
	uint8_t statistics_100_200_started=0; //stores id the statistic timer has started

	uint8_t printStopTheCar=0; //if =2 prints a message to screen for one second
	uint8_t printEnableDyno=0; //print the message "ENABLE DYNO" on the dashboard

	uint32_t shutdownDashboardMenuRequestTime=0; //used to shutdown display after one minute from motor off event

	uint8_t checkbox_symbols[2] = {0x4F, 0xD8}; // O (0x4F= not selected), Ø (0xD8= selected)

	//ACC_VIRTUAL_PAD
	uint8_t function_acc_virtual_pad_enabled=0; //default disabled. saved in flash
	//function Virtual ACC Pad
	CAN_TxHeaderTypeDef ACC_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2FA, .DLC=3};
	uint8_t ACC_msg_data[3];
	uint8_t newWheelPressedButtonID=0x10; //button released
	uint8_t ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=0;

	//IMMOBILIZER_ENABLED
	uint8_t immobilizerEnabled=1; //parameter stored in ram, so that we can change it dinamically
	uint8_t panicAlarmActivated=0; //indicates if the panic alarm was activated during last... 10 minutes (ToBeVerified)
	//the following 2 arrays declares: RFHUB reset (first message) and panic alarm messages definition (the others)
	CAN_TxHeaderTypeDef panicAlarmStartMsgHeader[5]={
			{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAC7F1, .DLC=3},
			{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x1E340041, .DLC=4},
			{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
			{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
	};

	uint8_t panicAlarmStartMsgData[5][8]={
			{0x02,0x11,0x01,0x00,},
			{0x88,0x20,0x15,0x00,},
			{0x42,0x02,0xE2,0x00,0x00,0x00,0x01,0x56},
			{0x00,0x00,0xE2,0x00,0x00,0x00,0x00,0x00},
	};

	CAN_TxHeaderTypeDef dashboardBlinkMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x545, .DLC=8};
	uint8_t dashboardBlinkMsgData[8]= {0x88, 0x20, 0xC3, 0x24, 0x00, 0x14, 0x30, 0x00};
	uint32_t last_sent_dashboard_blink_msg_time=0;
	uint8_t floodTheBus=0;
	uint32_t floodTheBusStartTime=0;
	uint32_t floodTheBusLastTimeSent=0;
	uint8_t executeDashboardBlinks=0; //executes the number of defined blinks, one each second.
	uint16_t engineOnSinceMoreThan5seconds=0; //if>=500 means engine is on since at least 5 seconds (for RFHUB too)


	//SMART_DISABLE_START_STOP
	//DISABLE_START_STOP
	uint8_t function_smart_disable_start_stop_enabled=1; //added to manage the function from the setup menu on the dashboard
	CAN_TxHeaderTypeDef disableStartAndStopMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x4B1, .DLC=8};
	uint8_t disableStartAndStopMsgData[8]= {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; //byte 5 shall be set to 0x08
	uint8_t startAndStopEnabled=1; //this is the status of my internal logic. If=0 the function goes to sleep up to next reboot
	uint8_t startAndstopCarStatus=1; //this is the status of Start&stop received by the car. 1=enabled in car (this is the default status in giulias).
	uint32_t lastTimeStartAndstopDisablerButtonPressed=0;
	uint8_t requestToDisableStartAndStop=0; //if set to 1 sends message simulating s&s button press

	//

	uint8_t main_dashboardPageIndex=0;
	uint8_t dashboard_menu_indent_level=0;
	uint8_t dashboard_main_menu_array_len=15;
	uint8_t dashboard_main_menu_array[20][DASHBOARD_MESSAGE_MAX_LENGTH]={
			{},
			{'S','h','o','w',' ','P','a','r','a','m','e','t','e','r','s',' ',' ',' '},
			{'R','e','a','d',' ','F','a','u','l','t','s',' ',' ',' ',' ',' ',' ',' '},
			{'C','l','e','a','r',' ','F','a','u','l','t','s',' ',' ',' ',' ',' ',' '},
			{'I','m','m','o','b','i','l','i','z','e','r',' ',' ','O','N',' ',' ',' '},
			{'T','o','g','g','l','e',' ','D','Y','N','O',' ',' ',' ',' ',' ',' ',' '},
			{'T','o','g','g','l','e',' ','E','S','C','/','T','C',' ',' ',' ',' ',' '},
			{'F','r','o','n','t',' ','B','r','a','k','e',' ','N','o','r','m','a','l'},
			{'4','W','D',' ',' ','E','n','a','b','l','e','d',' ',' ',' ',' ',' ',' '},
			{'M','a','i','n',' ','S','e','t','u','p',' ','M','e','n','u',' ',' ',' '},
			{'P','a','r','a','m','s',' ','S','e','t','u','p',' ','M','e','n','u',' '},
			{'E','n','a','b','l','e',' ','H','A','S',' ',' ',' ',' ',' ',' ',' ',' '},
			{'T','o','g','g','l','e',' ','Q','V',' ','V','a','l','v','e',' ',' ',' '},
			{'S','a','v','e',' ','L','o','g',' ','t','o',' ','F','i','l','e',' ',' '},
			{'R','e','s','e','t',' ','S','t','a','t','i','s','t','i','c','s',' ',' '},

	};
	uint8_t setup_dashboardPageIndex=0;
	uint8_t total_pages_in_setup_dashboard_menu=28;
	uint8_t dashboard_setup_menu_array[30][DASHBOARD_MESSAGE_MAX_LENGTH]={
			{'S','A','V','E','&','E','X','I','T',' ',' ',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','S','t','a','r','t','&','S','t','o','p',' ',' ',' ',' ',' '},
			{'L','a','u','n','c','h','T','o','r','q','u','e',' ','1','0','0','N','m'},
			{'O',' ',' ','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',' '},
			{'O',' ',' ','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
			{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',' ',' ',' ',' '},
			{'O',' ',' ','M','y','2','3',' ','I','P','C',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
			{0xD8,' ',' ','S','e','a','t','b','e','l','t',' ','A','l','a','r','m',' '},
			{'O',' ',' ','R','o','u','t','e',' ','M','e','s','s','a','g','e','s',' '},
			{'O',' ',' ','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',' '},
			{'O',' ',' ','D','y','n','o',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
			{'O',' ',' ','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
			{'O',' ',' ','4','W','D',' ','D','i','s','a','b','l','e','r',' ',' ',' '},
			{'O',' ',' ','C','l','e','a','r',' ','F','a','u','l','t','s',' ',' ',' '},
			{'O',' ',' ','R','e','a','d',' ',' ','F','a','u','l','t','s',' ',' ',' '},
			{'O',' ',' ','R','e','m','o','t','e',' ','S','t','a','r','t',' ',' ',' '},
			{0xD8,' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
			{'O',' ',' ','P','e','d','a','l',' ','B','o','o','s','t','e','r',' ',' '},
			{'O',' ',' ','O','d','o','m','e','t','e','r',' ','B','l','i','n','k',' '},
			{'O',' ',' ','S','h','o','w',' ','R','a','c','e',' ','M','a','s','k',' '},
			{'O',' ',' ','P','a','r','k',' ','M','i','r','r','o','r',' ',' ',' ',' '},
			{'O',' ',' ','A','C','C',' ','A','u','t','o','s','t','a','r','t',' ',' '},
			{'O',' ',' ','C','l','o','s','e',' ','W','i','n','d','o','w','s',' ',' ',},
			{'O',' ',' ','O','p','e','n',' ',' ','W','i','n','d','o','w','s',' ',' ',},
			{'O',' ',' ','H','A','S',' ','V','i','r','t','u','a','l',' ','P','a','d',},
			{'O',' ',' ','Q','V',' ','E','x','h','a','u','s','t',' ','F','l','a','p',},

		};

	uint8_t function_is_diesel_enabled=1; //stored in flash. defines if we use gasoline (0) or diesel (1) params


	CAN_TxHeaderTypeDef uds_parameter_request_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA10F1, .DLC=3};
	uint8_t baccableDashboardMenuVisible=0;
	uint8_t baccabledashboardMenuWasVisible=0; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
	uint8_t oilPressure; 		//oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
	uint8_t oilLevel;			//oil level from 0 to 14.
	uint8_t oilTemperature; 	//oil temperature in celsious degrees (to correct by offset)
	uint8_t waterTemperature; 	//water temperature in celsius degrees (to correct by offset)
	int16_t torque; 			//torque
	uint8_t batteryStateOfCharge=0; //battery charge %
	uint16_t batteryCurrent; 		//battery current (to be converted in Amps)
	uint8_t transmissionTemperature;
	uint8_t uds_parameter_request_msg_data[8];
	uint8_t dashboardPageIndex=0; //to send message index - it changes when you press cruise control buttons
	uint32_t last_sent_uds_parameter_request_Time=0; //stores last time we send a uds parameter request

	uint8_t dieselEngineRegenerationMode=0; //0=None, 1=DPF_REGEN_LO, 2=DPF_REGEN_HI, 3=NSC_DE_NOX_REGEN, 4=NSC_DE_SOX_REGEN, 5=SCR_HEATUP_STRATEGY

	//
	uint8_t cruiseControlDisabled=1;
	uint8_t ACC_Disabled=1;
	uint8_t ACC_engaged=0;
	uint8_t wheelPressedButtonID=0x10; //0x10= released, 0x20=strong speed decrease, 0x18=speed decrease, 0x00=strong speed increase, 0x08=speed increase, 0x90=RES, CC on/off=0x12
	uint8_t  lastPressedWheelButton=0xff; //default value, means no button pressed on the wheel
	uint32_t lastPressedWheelButtonTime=0;//stores the last time a wheel button was pressed, in msec from boot
	uint32_t lastPressedWheelButtonDuration=0x00; //default value
	uint32_t lastPressedSpeedUpWheelButtonDuration=0x00; //default value

	//ROUTE_MSG
	uint8_t function_route_msg_enabled=0; //default disabled . stored in flash
	uint8_t routeStdIdMsg=0xff;
	uint8_t routeOffset=0;
	uint32_t routeMsgId=0;
	CAN_TxHeaderTypeDef routeMsgHeader={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAF1BA, .DLC=8};
	uint8_t routeMsgData[8]= {0x07, 0x62, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

	//REMOTE_START_ENABLED
	uint8_t function_remote_start_Enabled=0; //default disabled . stored in flash
	uint8_t engineRemoteStartRequest=0;
	CAN_TxHeaderTypeDef REMOTE_START_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8};
	uint8_t REMOTE_START_msg_data[8]={0x42,0x04,0x96,0,};
	uint8_t pressStartButton=0;
	CAN_TxHeaderTypeDef BODY4_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8};


	//READ_FAULTS_ENABLED
	uint8_t function_read_faults_enabled=1;

	//_4WD_DISABLER_ENABLED
	uint8_t function_4wd_disabler_enabled=1; //default enabled . stored in flash
	CAN_TxHeaderTypeDef driveTrainControlModuleResetMsgHeader[4]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=7},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3}};
	uint8_t driveTrainControlModuleResetMsgData[4][8]= {{0x02, 0x11, 0x01,},{0x06, 0x2F, 0x2A, 0xAA, 0x03, 0x00, 0x00,},{0x02, 0x3E, 0x80,},{0x02, 0x10, 0x03,}}; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(set front torque to 0), reset ECU
	uint32_t last_sent_drive_train_msg_time=0;

	//IPC_MY23_IS_INSTALLED
	uint8_t function_ipc_my23_is_installed=1;

	//REGENERATION_ALERT_ENABLED
	uint8_t function_regeneration_alert_enabled=0;
	uint8_t regenerationInProgress=0;
	//uint8_t STATUS_ECM_msg_data[8];
	//CAN_TxHeaderTypeDef STATUS_ECM_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5AE, .DLC=8};
	uint8_t loopsFromRegenerationEnded=0;

	//SHIFT_INDICATOR_ENABLED
	uint8_t function_shift_indicator_enabled=0; //saved in flash.
	CAN_TxHeaderTypeDef shift_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2ED, .DLC=8};
	uint8_t shift_msg_data[8];

	#if defined(SHIFT_THRESHOLD)
		uint16_t shift_threshold=SHIFT_THRESHOLD;
	#else
		uint16_t shift_threshold=1500;
	#endif

	//LED_STRIP_CONTROLLER_ENABLED
	uint8_t function_led_strip_controller_enabled=0; //defines is not enough, by default leds are disabled.stored in ram
	float scaledVolume;
	uint8_t scaledColorSet;
	uint32_t timeSinceLastReceivedAcceleratorMessage=0;
	uint8_t ledsStripIsOn=0; //indicates if leds strip is on

	//DYNO_MODE_MASTER
	uint8_t function_dyno_mode_master_enabled=1; //stored in flash


	//FRONT_BRAKE_FORCER_MASTER
	uint8_t function_front_brake_forcer_master=1; //stored in flash

	#if defined(LAUNCH_ASSIST_THRESHOLD)
		uint16_t launch_torque_threshold=LAUNCH_ASSIST_THRESHOLD;
	#else
		uint16_t launch_torque_threshold=100;
	#endif


	//SEAT BELT DISABLE messages and function
	// diag session request 	0x02,0x10,0x03
	// diag session reply 		0x06,0x50,0x03,0x00,0x32,0x01,0xF4
	// disable seatbelt			0x05,0x2F,0x55,0xA0,0x03,0x00
	// en/dis seatbelt	reply	0x04,0x6F,0x55,0xA0,0x03
	// enable seatbelt			0x05,0x2F,0x55,0xA0,0x03,0x01
	uint8_t function_seatbelt_alarm_enabled=0;
	uint8_t seatbeltAlarmDisabled=0xff;  //FF unknown, 0x10 sent diag session....
	uint32_t seatbeltAlarmStatusRequestTime=0;
	// Values associated to the variable seatbeltAlarmDisabled:
	// ff=undefined,
	// fe=status in Aquisition,
	// 0=seatbeltAlarmEnabled,
	// 1=seatBeltAlarmDisabled,
	// 0x10=request to disable SeatBelt alarm in progress(open diag session was sent),
	// 0x11=request to disable SeatBelt alarm in progress(write param was sent),

	// 0x20=request to enable Seatbelt Alarm in progress(open diag session was sent)
	// 0x21=request to enable Seatbelt Alarm in progress(write param was sent)


	CAN_TxHeaderTypeDef seatBeltMsgHeader[2]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA60F1, .DLC=6},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA60F1, .DLC=3}};
	uint8_t seatbeltMsgData[2][8]= {{0x05,0x2F,0x55,0xA0,0x03,0x00,},{0x02, 0x10, 0x03,}}; //from last to first we have: diag session, short term adjustment

	//DISABLE_ODOMETER_BLINK
	uint8_t function_disable_odometer_blink=0;

	uint8_t instructSlaveBoardsTriggerEnabled=0; //variable to send messages to C2 and BH chips, after a wakeup event

	//ACC Autostart
	uint8_t function_acc_autostart=0;
	uint32_t lastSentAutostartMsg=0;
	uint8_t AutostartMsgCounter=0;
	uint8_t carSteadyCounter=0; //tells how many seconds car is steady (200 max value = 2000msec)
	uint8_t brakeIntervention_ACC_ESC_ASR=0; //tells if brake is pressed

	//CLOSE_WINDOWS
	uint8_t function_close_windows_with_door_lock=0; //0=disabled, 1=close windows1, 2=close windows2
	uint8_t closeWindowsRequest=0;  //0=disabled, 1=close windows, 2=windows ajar
	uint32_t doorCloseTime=0;
	uint8_t doorLocksRequestsCounter=0;

	uint8_t function_open_windows_with_door_lock=0; //0=disabled, 1=open windows1, 2=open windows2
	uint8_t openWindowsRequest=0;  //0=disabled, 1=open windows
	uint32_t doorOpenTime=0;
	uint8_t doorUnlocksRequestsCounter=0;

	uint8_t RF_requestor=0;
	uint8_t RF_fob_number=0;

	//QV_EXHAUST_FLAP_FUNCTION_ENABLED
	uint8_t QV_exhaust_flap_function_enabled=0;
	uint8_t ForceQVexhaustValveOpened=0; //state machine: 0=valve automatically selected by ECU, 1=connection request to send, 2=tester present to send, 3=temporary param overwrite, 4=return control to ECU to send
	uint32_t lastSentQVexhaustValveMsgTime=0;
	CAN_TxHeaderTypeDef forceQVexhaustValveMsgHeader[4] ={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA17F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA17F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA17F1, .DLC=7},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA17F1, .DLC=5}};
	uint8_t forceQVexhaustValveMsgData[4][8]= {{0x02, 0x10, 0x03,},{0x02, 0x3E, 0x00,}, {0x06, 0x2F, 0x51, 0x90, 0x03, 0x00, 0x00,},{0x04, 0x2F, 0x51, 0x90, 0x00,}}; //from first to last we have: diag session, tester present, IO Control - Short Term Adjustment(open Valve) (periodic), return control to ECU
	uint8_t numberOfReleaseButtonClicks=0;
	uint32_t ReleasebuttonFirstClickTime=0;
	uint32_t ReleasebuttonPressBeginTime=0;


#endif

#if defined(C2baccable)
	//DYNO_MODE

	uint8_t DynoModeEnabled=0;
	uint8_t DynoStateMachine=0xff; //State machine for dyno messages sequence. frm 00 to 03 = dyno message sequence is beeing transmitted. FF= inactive
	uint16_t testerMsgSent=0;
	uint8_t DYNO_msg_data[5][6]={{0x02,0x10,0x03,},{0x03,0x22,0x30,0x02,},{0x05,0x2E,0x30,0x02,0x00,0x01},{0x05,0x2E,0x30,0x02,0xFF,0x01}, {0x02, 0x3E, 0x80,}}; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno, index 4=tester presence
	CAN_TxHeaderTypeDef DYNO_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=6};
	uint32_t last_sent_tester_presence_msg_time=0; //stores time in millisec. from last sent presence. used when dyno is enabled
	uint32_t DynoStateMachineLastUpdateTime=0; //stores time (in milliseconds from power on) when Park Assist button press was read last time
	uint8_t ParkAssistButtonPressCount=0; //stores number of times this message field was received

	//FRONT_BRAKE_FORCER
	uint32_t last_sent_rear_brake_msg_time=0;

	CAN_TxHeaderTypeDef rearBrakeMsgHeader[4]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=5},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=8},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3}};
	uint8_t rearBrakeMsgData[4][8]= {{0x04, 0x2F, 0x5A, 0xBD, 0x00,},{0x07, 0x2F, 0x5A, 0xBD, 0x03, 0x27, 0x10, 0x03},{0x02, 0x3E, 0x80,},{0x02, 0x10, 0x40,}}; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(disable front brakes) (periodic)

#endif

#if defined(BHbaccable)
	uint32_t lastSentTelematic_display_info_msg_Time=0;
	uint8_t telematic_display_info_field_totalFrameNumber=(DASHBOARD_MESSAGE_MAX_LENGTH / 3) - 1; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5)
	uint8_t telematic_display_info_field_frameNumber=0; //current frame
	uint8_t telematic_display_info_field_infoCode=DISPLAY_INFO_CODE;
	uint8_t paramsStringCharIndex=0; // next char to send index
	CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8};
	uint8_t telematic_display_info_msg_data[8];
	uint8_t requestToSendOneFrame=0; //set to 1 to send one frame on dashboard

	//Message to generate sound indication (chime)
	uint8_t CHIME_msg_data[8];
	CAN_TxHeaderTypeDef CHIME_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5AC, .DLC=8};
	uint8_t requestToPlayChime=0;

	#if defined(DISABLE_ODOMETER_BLINK)
		uint8_t disable_odometer_blink=1;
	#else
		uint8_t disable_odometer_blink=0;
	#endif
#endif

//ESC_TC_CUSTOMIZATOR_MASTER)
uint8_t function_esc_tc_customizator_enabled=0; //stored in flash

//ESC_TC_CUSTOMIZATOR_ENABLED
//uint32_t LANEbuttonPressLastTimeSeen=0; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
//uint32_t LANEbutton2PressLastTimeSeen=0; //like previous one but for lane button on my20 cars. Stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
uint32_t LANEbuttonPressBeginTime=0;
uint32_t LANEbuttonFirstClickTime=0;
//uint8_t LANEbuttonPressCount=0; //stores number of times this message field was received
//uint8_t LANEbutton2PressCount=0; //like previous one but for lane button on my20 cars. Stores number of times this message field was received
uint8_t numberOfLaneButtonClicks=0; //number of times button was pressed(click)

//PEDAL_BOOSTER_ENABLED
uint8_t function_pedal_booster_enabled=0; //0=disabled, 1=Automatic Map, 2=Bypass, 3=All Weather Map, 4=Natural Map, 5=Dynamic Map, 6=Race Map

//CLEAR_FAULTS_ENABLED
uint8_t function_clear_faults_enabled=1; //default enabled. saved on flash
uint8_t clearFaultsRequest=0; //if enabled, sends messages to clear faults
uint32_t last_sent_clear_faults_msg=0;
uint8_t clearFaults_msg_data[5]={0x04,0x14,0xff,0xff,0xff}; //message to clear DTC
CAN_TxHeaderTypeDef clearFaults_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA00F1, .DLC=5};

//
uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]={' ',}; //it contains string to print on dashboard

float currentSpeed_km_h=0; //current vehicle speed
float previousSpeed_km_h=0; //store speed at previous loop
uint32_t statistics_0_100_StartTime=0;
uint32_t statistics_100_200_StartTime=0;

uint32_t weCanSendAMessageReply=0; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
uint32_t currentTimeMainLoopDebug; //stores current time in milliseconds, each time we enter the main loop

UART_HandleTypeDef huart2; // this is the serial line between baccables

uint32_t currentRpmSpeed=0;	//used by C1baccable
uint8_t currentGear=0; 		//used by C1baccable and BHbaccable

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0,};  //msg data

uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

uint8_t _4wd_disabled=0; //if =4 disables 4wd
uint8_t front_brake_forced=0; //if=5 disables Front brakes
uint8_t DynoModeEnabledOnMaster=0; //status of dyno in master board. tells if dyno is active

uint8_t launch_assist_enabled=0; //if=1 assist is enabled and uses torque as trigget to release front brakes
uint8_t commandsMenuEnabled=1; //if 0 disables the up-down buttons to change menu position

//LOW CONSUME
uint8_t lowConsumeIsActive=0; //0=false, 1=true
uint32_t lastReceivedCanMsgTime=0;
uint32_t allProcessorsWakeupTime=0;

//ESC/TC function (common to C1,C2,BH
uint8_t currentDNAmode; //on C1 and C2 bus 0x00=Natural, 0x08=dynamic 0x10=AllWeather, 0x30=race (but on BH bus 0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
uint8_t DNA_msg_data[8];
CAN_TxHeaderTypeDef DNA_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8};
uint32_t lastSent384=0;
uint8_t ESCandTCinversion=0; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)

//SHOW RACE MASK
uint8_t function_show_race_mask=0;

//PARK_MIRROR
uint8_t function_park_mirror=0;
uint8_t leftMirrorHorizontalOperativePos=0;		//current Operative position
uint8_t leftMirrorVerticalOperativePos=0; 		//current Operative position
uint8_t rightMirrorHorizontalOperativePos=0;	//current Operative position
uint8_t rightMirrorVerticalOperativePos=0;		//current Operative position
uint8_t storeOperativeMirrorPosition=0;	//get current Operative position boolean
uint8_t leftParkMirrorVerticalPos=0; 	//Stored Park position
uint8_t leftParkMirrorHorizontalPos=0;	//Stored Park position
uint8_t rightParkMirrorVerticalPos=0;	//Stored Park position
uint8_t rightParkMirrorHorizontalPos=0; //Stored Park position
uint8_t storeCurrentParkMirrorPosition=0;//Store Park mirrors position Request
uint8_t parkMirrorsSteady=1; // park mirrors are not moving if this is =1
uint8_t turnIndicator=0; //0= center, 1=right, 2=left
uint8_t parkMirrorMsgData[8]={0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00};
CAN_TxHeaderTypeDef parkMirrorMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5A8, .DLC=8};
uint32_t lastParkMirrorMsgTime=0;
uint32_t restoreOperativeMirrorsPositionRequestTime=0;
uint8_t restoreOperativeMirrorsPosition=0;
uint8_t leftParkMirrorPositionRequired=0;
uint8_t rightParkMirrorPositionRequired=0;
uint8_t parkMirrorOperativePositionNotStored=1;

//HAS_FUNCTION_ENABLED
uint8_t HAS_function_enabled=0;
uint8_t HAS_buttonPressRequested=0;

uint8_t neverSaved=1;
uint8_t usbInited;

uint32_t lastUartErrorCallback;
