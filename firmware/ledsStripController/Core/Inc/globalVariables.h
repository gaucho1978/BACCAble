/*
 * globalVariables.h
 *
 *  Created on: Apr 27, 2025
 *      Author: GauchoHP
 */

#ifndef INC_GLOBALVARIABLES_H_
	#define INC_GLOBALVARIABLES_H_
	#include "compile_time_defines.h"
	#include "stm32f0xx_hal.h"







	#define ENGINE_NORMAL				0
	#define ENGINE_DPF_REGEN_LO			1
	#define ENGINE_DPF_REGEN_HI			2
	#define ENGINE_NSC_DE_NOX_REGEN		3
	#define ENGINE_NSC_DE_SOX_REGEN		4
	#define ENGINE_SCR_HEATUP_STRATEGY	5


	//this is used to invert bytes order in a 32 bit integer
	#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) | \
						   (((uint32_t)(x) >> 8)  & 0x0000FF00) | \
						   (((uint32_t)(x) << 8)  & 0x00FF0000) | \
						   (((uint32_t)(x) << 24) & 0xFF000000)

	#define LAST_PAGE_ADDRESS (FLASH_BANK1_END - FLASH_PAGE_SIZE +1) // 0x0801F800 //valid only for stm32F072 i suppose
			//la flash inizia a 0x08000000  e finisce a 0x0801FFFF, -0x800 +1 di una pagina fa 0x0801F800
	#define LAST_PAGE_ADDRESS_STATISTICS LAST_PAGE_ADDRESS- FLASH_PAGE_SIZE //we will use previous page for statistics

#if defined(ACT_AS_CANABLE) ||  defined(DEBUG_MODE)
	//#include "usbd_def.h"
	#include "usb_device.h"
	#include "string.h"
	#include "usbd_cdc_if.h"

#endif


	#include "onboardLed.h"
	#include "can.h"
	#include "slcan.h"
	#include "math.h"


	#include "uart.h"

	#if defined(C1baccable)
		#include "vuMeter.h" //this is used to control led strip through usb pin
		#include "lowConsume.h"
		extern uint32_t lastReceivedCanMsgTime;
	#endif


	#if defined(C1baccable)
		typedef struct{
			uint8_t		name[DASHBOARD_MESSAGE_MAX_LENGTH - 3];
			uint32_t 	reqId;
			uint8_t		reqLen;
			uint32_t 	reqData;
			uint32_t 	replyId;
			uint8_t		replyLen;
			uint8_t		replyOffset;
			int32_t		replyValOffset;
			float		replyScale;
			int32_t		replyScaleOffset;
			uint8_t		replyMeasurementUnit[7];
			uint8_t		replyDecimalDigits;

		} uds_param_element;

	#endif


		//all variables of globalVariables.c shall be repeated here as extern
		extern const char *FW_VERSION;
		extern const uint8_t led_light_on_bit;



	#if defined(C1baccable)
		extern float chronometerElapsedTime_0_100_km_h; //stores time statistic in seconds
		extern float chronometerElapsedTime_100_200_km_h; //stores time statistic in seconds
		extern uint8_t statistics_0_100_started; //stores id the statistic timer has started
		extern uint8_t statistics_100_200_started; //stores id the statistic timer has started
		extern uint8_t printStopTheCar; //if =2 prints a message to screen for one second
		extern uint8_t printEnableDyno; //print the message "ENABLE DYNO" on the dashboard

		extern uint32_t shutdownDashboardMenuRequestTime; //used to shutdown display after one minute from motor off event
		extern uint8_t checkbox_symbols[2]; // O (0x4F= not selected), Ø (0xD8= selected)
		//ACC_VIRTUAL_PAD
		extern uint8_t function_acc_virtual_pad_enabled; //default disabled. saved in flash
		//function Virtual ACC Pad
		extern CAN_TxHeaderTypeDef ACC_msg_header;
		extern uint8_t ACC_msg_data[3];
		extern uint8_t newWheelPressedButtonID; //button released
		extern uint8_t ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED;

		//IMMOBILIZER_ENABLED
		extern uint8_t immobilizerEnabled; //parameter stored in ram, so that we can change it dinamically
		extern uint8_t panicAlarmActivated; //indicates if the panic alarm was activated during last... 10 minutes (ToBeVerified)
		//the following 2 arrays declares: RFHUB reset (first message) and panic alarm messages definition (the others)
		extern CAN_TxHeaderTypeDef panicAlarmStartMsgHeader[5];
		extern uint8_t panicAlarmStartMsgData[5][8];

		extern CAN_TxHeaderTypeDef dashboardBlinkMsgHeader;
		extern uint8_t dashboardBlinkMsgData[8];
		extern uint32_t last_sent_dashboard_blink_msg_time;
		extern uint8_t floodTheBus;
		extern uint32_t floodTheBusStartTime;
		extern uint32_t floodTheBusLastTimeSent;
		extern uint8_t executeDashboardBlinks; //executes the number of defined blinks, one each second.
		extern uint16_t engineOnSinceMoreThan5seconds; //if>=500 means engine is on since at least 5 seconds


		//SMART_DISABLE_START_STOP
		//DISABLE_START_STOP
		extern uint8_t function_smart_disable_start_stop_enabled; //added to manage the function from the setup menu on the dashboard
		extern CAN_TxHeaderTypeDef disableStartAndStopMsgHeader;
		extern uint8_t disableStartAndStopMsgData[8]; //byte 5 shall be set to 0x08
		extern uint8_t startAndStopEnabled; //this is the status of my internal logic. If=0 the function goes to sleep up to next reboot
		extern uint8_t startAndstopCarStatus; //this is the status of Start&stop received by the car. 1=enabled in car (this is the default status in giulias).
		extern uint32_t lastTimeStartAndstopDisablerButtonPressed;
		extern uint8_t requestToDisableStartAndStop; //if set to 1 sends message simulating s&s button press

		//
		extern uint8_t gearArray[11];

		extern uint8_t main_dashboardPageIndex;
		extern uint8_t dashboard_menu_indent_level;
		extern uint8_t dashboard_main_menu_array_len;
		extern uint8_t dashboard_main_menu_array[10][DASHBOARD_MESSAGE_MAX_LENGTH];

		extern uint8_t setup_dashboardPageIndex;
		extern uint8_t total_pages_in_setup_dashboard_menu;
		extern uint8_t dashboard_setup_menu_array[25][DASHBOARD_MESSAGE_MAX_LENGTH];

		extern uint8_t function_is_diesel_enabled; //stored in flash. defines if we use gasoline (0) or diesel (1) params
		extern uint8_t total_pages_in_dashboard_menu_diesel;
		extern uint8_t total_pages_in_dashboard_menu_gasoline;
		// uds_params_array[0] contais gasoline params, , uds_params_array[1] contains diesel params
		extern const	uds_param_element uds_params_array[2][60]; // initializes all the uds parameters request to send and reply to receive

		extern CAN_TxHeaderTypeDef uds_parameter_request_msg_header;
		extern uint8_t baccableDashboardMenuVisible;
		extern uint8_t baccabledashboardMenuWasVisible; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
		extern uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
		extern uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
		extern int16_t torque; //torque
		extern uint8_t batteryStateOfCharge; //battery charge %
		extern uint16_t batteryCurrent; //battery current (to be converted in Amps)
		extern uint8_t transmissionTemperature;
		extern uint8_t uds_parameter_request_msg_data[8];
		extern uint8_t dashboardPageIndex; //to send message index - it changes when you press cruise control buttons
		extern uint32_t last_sent_uds_parameter_request_Time; //stores last time we send a uds parameter request

		extern uint8_t dieselEngineRegenerationMode; //0=None, 1=DPF_REGEN_LO, 2=DPF_REGEN_HI, 3=NSC_DE_NOX_REGEN, 4=NSC_DE_SOX_REGEN, 5=SCR_HEATUP_STRATEGY

		//
		extern uint8_t cruiseControlDisabled;
		extern uint8_t ACC_Disabled;
		extern uint8_t ACC_engaged;
		extern uint8_t wheelPressedButtonID; //0x10= released, 0x20=strong speed decrease, 0x18=speed decrease, 0x00=strong speed increase, 0x08=speed increase, 0x90=RES, CC on/off=0x12
		extern uint8_t  lastPressedWheelButton; //default value, means no button pressed on the wheel
		extern uint32_t lastPressedWheelButtonTime;//stores the last time a wheel button was pressed, in msec from boot
		extern uint32_t lastPressedWheelButtonDuration; //default value
		extern uint32_t lastPressedSpeedUpWheelButtonDuration; //default value

		//ROUTE_MSG
		extern uint8_t function_route_msg_enabled; //default disabled . stored in flash
		extern uint8_t routeStdIdMsg;
		extern uint8_t routeOffset;
		extern uint32_t routeMsgId;
		extern CAN_TxHeaderTypeDef routeMsgHeader;
		extern uint8_t routeMsgData[8];

		//REMOTE_START_ENABLED
		extern uint8_t function_remote_start_Enabled; //default disabled . stored in flash
		extern uint8_t engineRemoteStartRequest;
		extern uint32_t doorOpenTime;
		extern CAN_TxHeaderTypeDef REMOTE_START_msg_header;
		extern uint8_t REMOTE_START_msg_data[8];
		extern uint8_t RF_fob_number;
		extern uint8_t pressStartButton;
		extern CAN_TxHeaderTypeDef BODY4_msg_header;


		//READ_FAULTS_ENABLED
		extern uint8_t function_read_faults_enabled;

		//_4WD_DISABLER_ENABLED
		extern uint8_t function_4wd_disabler_enabled; //default enabled . stored in flash
		extern CAN_TxHeaderTypeDef driveTrainControlModuleResetMsgHeader[4];
		extern uint8_t driveTrainControlModuleResetMsgData[4][8]; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(set front torque to 0), reset ECU
		extern uint32_t last_sent_drive_train_msg_time;

		//IPC_MY23_IS_INSTALLED
		extern uint8_t function_ipc_my23_is_installed;

		//REGENERATION_ALERT_ENABLED
		extern uint8_t function_regeneration_alert_enabled;
		extern uint8_t regenerationInProgress;
		extern uint8_t STATUS_ECM_msg_data[8];
		extern CAN_TxHeaderTypeDef STATUS_ECM_msg_header;

		//SHIFT_INDICATOR_ENABLED
		extern uint8_t function_shift_indicator_enabled; //saved in flash.
		extern CAN_TxHeaderTypeDef shift_msg_header;
		extern uint8_t shift_msg_data[8];

		#if defined(SHIFT_THRESHOLD)
			extern uint16_t shift_threshold;
		#else
			extern uint16_t shift_threshold;
		#endif

		//LED_STRIP_CONTROLLER_ENABLED
		extern uint8_t function_led_strip_controller_enabled; //defines is not enough, by default leds are disabled.stored in ram
		extern float scaledVolume;
		extern uint8_t scaledColorSet;
		extern uint32_t timeSinceLastReceivedAcceleratorMessage;
		extern uint8_t ledsStripIsOn; //indicates if leds strip is on

		//DYNO_MODE_MASTER
		extern uint8_t function_dyno_mode_master_enabled; //stored in flash

		//ESC_TC_CUSTOMIZATOR_MASTER)
		extern uint8_t function_esc_tc_customizator_enabled; //stored in flash

		//FRONT_BRAKE_FORCER_MASTER
		extern uint8_t function_front_brake_forcer_master; //stored in flash
		extern uint16_t launch_torque_threshold; //stored in flash

		extern uint32_t lastReceivedCanMsgTime; //this is not in global C file, but comes from another C file

		//SEAT BELT DISABLE messages and function
		// diag session request 	0x02,0x10,0x03
		// diag session reply 		0x06,0x50,0x03,0x00,0x32,0x01,0xF4
		// disable seatbelt			0x05,0x2F,0x55,0xA0,0x03,0x00
		// en/dis seatbelt	reply	0x04,0x6F,0x55,0xA0,0x03
		// enable seatbelt			0x05,0x2F,0x55,0xA0,0x03,0x01
		extern uint8_t function_seatbelt_alarm_enabled;
		extern uint8_t seatbeltAlarmDisabled;
		// Values associated to the variable seatbeltAlarmDisabled:
		// ff=undefined,
		// fe=status in Aquisition,
		// 0=seatbeltAlarmEnabled,
		// 1=seatBeltAlarmDisabled,
		// 0x10=request to disable SeatBelt alarm in progress(write param shall be sent),
		// 0x20=request to enable Seatbelt Alarm in progress(write param shall be sent)

		extern uint32_t seatbeltAlarmStatusRequestTime;
		extern CAN_TxHeaderTypeDef seatBeltMsgHeader[2];
		extern uint8_t seatbeltMsgData[2][8];
		//extern uint32_t last_sent_seatbelt_msg_time;

	#endif

	#if defined(C2baccable)
		//ESC_TC_CUSTOMIZATOR_ENABLED
		extern uint8_t currentDNAmode;
		extern uint8_t DNA_msg_data[8];
		extern CAN_TxHeaderTypeDef DNA_msg_header;
		extern uint8_t ESCandTCinversion; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)
		extern uint32_t LANEbuttonPressLastTimeSeen; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
		extern uint8_t LANEbuttonPressCount; //stores number of times this message field was received

		//DYNO_MODE
		extern uint8_t DynoModeEnabled;
		extern uint8_t DynoStateMachine; //State machine for dyno messages sequence. frm 00 to 03 = dyno message sequence is beeing transmitted. FF= inactive
		extern uint16_t testerMsgSent;
		extern uint8_t DYNO_msg_data[5][6]; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno, index 4=tester presence
		extern CAN_TxHeaderTypeDef DYNO_msg_header;
		extern uint32_t last_sent_tester_presence_msg_time; //stores time in millisec. from last sent presence. used when dyno is enabled
		extern uint32_t DynoStateMachineLastUpdateTime; //stores time (in milliseconds from power on) when Park Assist button press was read last time
		extern uint8_t ParkAssistButtonPressCount; //stores number of times this message field was received

		//FRONT_BRAKE_FORCER
		extern uint32_t last_sent_rear_brake_msg_time;

		extern CAN_TxHeaderTypeDef rearBrakeMsgHeader[4];
		extern uint8_t rearBrakeMsgData[4][8]; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(disable front brakes) (periodic)

	#endif

	#if defined(BHbaccable)
		extern uint32_t lastSentTelematic_display_info_msg_Time;
		extern uint8_t telematic_display_info_field_totalFrameNumber; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5)
		extern uint8_t telematic_display_info_field_frameNumber; //current frame
		extern uint8_t telematic_display_info_field_infoCode;
		extern uint8_t paramsStringCharIndex; // next char to send index.
		extern CAN_TxHeaderTypeDef telematic_display_info_msg_header;
		extern uint8_t telematic_display_info_msg_data[8];
		extern uint8_t requestToSendOneFrame; //set to 1 to send one frame on dashboard

		//Message to generate sound indication (chime)
		extern uint8_t CHIME_msg_data[8];
		extern CAN_TxHeaderTypeDef CHIME_msg_header;
		extern uint8_t requestToPlayChime;

	#endif

		//CLEAR_FAULTS_ENABLED
		extern uint8_t function_clear_faults_enabled; //default enabled. saved on flash
		extern uint8_t clearFaultsRequest; //if enabled, sends messages to clear faults
		extern uint32_t last_sent_clear_faults_msg;
		extern uint8_t clearFaults_msg_data[5]; //message to clear DTC
		extern CAN_TxHeaderTypeDef clearFaults_msg_header;

		//
		extern uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]; //it contains string to print on dashboard

		extern float currentSpeed_km_h; //current vehicle speed
		extern float previousSpeed_km_h; //store speed at previous loop
		extern uint32_t statistics_0_100_StartTime;
		extern uint32_t statistics_100_200_StartTime;

		extern uint32_t weCanSendAMessageReply; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
		extern uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
		extern uint32_t currentTime; //stores current time in milliseconds, each time we enter the main loop

		extern UART_HandleTypeDef huart2; // this is the serial line between baccables

		extern uint32_t currentRpmSpeed;	//used by C1baccable
		extern uint8_t currentGear; 		//used by C1baccable

		// Storage for status and received message buffer
		extern CAN_RxHeaderTypeDef rx_msg_header;  //msg header
		extern uint8_t rx_msg_data[8];  //msg data

		extern uint8_t msg_buf[]; //msg converted in ascii to send over usb

		extern uint8_t _4wd_disabled; //if =4 disables 4wd
		extern uint8_t front_brake_forced; //if=5 disables Front brakes
		extern uint8_t DynoModeEnabledOnMaster; //status of dyno in master board. tells if dyno is active

		extern uint8_t launch_assist_enabled; //if=1 assist is enabled and uses torque as trigget to release front brakes

		extern uint8_t commandsMenuEnabled; //if 0 disables the up-down buttons to change menu position

#endif /* INC_GLOBALVARIABLES_H_ */
