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

	//This section sets ACT_AS_CANABLE if no option was selected
	#if !defined(ACT_AS_CANABLE) && !defined(C1baccable) && !defined(C2baccable) && !defined(BHbaccable)
		#define ACT_AS_CANABLE
	#endif

	#if defined(ACT_AS_CANABLE)
		#pragma message("Building CANable") //adds a message in the compilation log
	#endif

	#if defined(C1baccable) //this works only on C1 can bus (OBD port pins 6 and 14)
		#pragma message("Building C1 BACCAble") //adds a message in the compilation log

		#define LOW_CONSUME //master baccable will put other 2 chips and the other 2 can transceivers to sleep.

		//if one of the following will be uncommented, its default status will be enabled. It will be possible to change the status inside SETUP menu.
		#define IMMOBILIZER_ENABLED //immobilizer will be enabled.
		#define SMART_DISABLE_START_STOP //start&stop will be automatically disabled with can message
		//#define DISABLE_START_STOP //start&stop disabling with external resistor simulating button press. this is left just for reference. no more used.
		//#define LED_STRIP_CONTROLLER_ENABLED //led strip controller functionality
		//#define SHIFT_INDICATOR_ENABLED //show shift indicator when rpm motor goes over the configurable threshold SHIFT_THRESHOLD (in race mode)
		#define SHIFT_THRESHOLD 4500 //default shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		//#define IPC_MY23_IS_INSTALLED //this is used in SHIFT_INDICATOR_ENABLED functionality, if you are using IPC for My23 Giulia/Stelvio
		#define SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE //Used if you connected another baccable to usb port and want this baccable to send parameters to slave baccable (the slave will display parameter on the dashboard). if defined, the cruise control buttons + and - will change the shown parameter
		//#define ROUTE_MSG // allows communication with  commercial diagnostic tools to supply internal bus parameters. better described in the manual
		//#define DYNO_MODE_MASTER //allows to request DYNO in master baccable
		//#define ESC_TC_CUSTOMIZATOR_MASTER //used to activate ESC/TC in master baccable
		//#define ACC_VIRTUAL_PAD //simulate the presence of ACC button pad. When the user press button to enable/disable CC, the BACCABLE sends message to enable/disable ACC
		//#define FRONT_BRAKE_FORCER_MASTER //used to activate FRONT_BRAKE_FORCER in master baccable
		//#define _4WD_DISABLER //disables 4WD
		#define CLEAR_FAULTS_ENABLED //baccable menu will allow to reset faults on all 3 baccable

		//experimental: it still do not work. don't use it!
		//#define READ_FAULTS_ENABLED // baccable menu will allow to read faults

		//experimental: it still do not work. don't use it!
		//#define REMOTE_START_ENABLED //the car can be powered on by remote by means of the original key

		// IS_GASOLINE  is defined if compiling with eclipse (on stm cube it will be not defined)
		#ifndef IS_GASOLINE
			#pragma message("Will select default diesel engine parameters")
			//If compiling with STM cube IDE, you will have to comment and uncomment the following line:
			#define IS_DIESEL //if uncommented sets by default diesel parameters (you can change it in setup menu), otherwise Gasoline will be default setting.
		#else
			#pragma message("Will select default gasoline engine parameters")
		#endif

		//#define REGENERATION_ALERT_ENABLED //if enabled, an alert wil be fired during each DPF regeneration process


	#endif

	#if defined(C2baccable) //this works only on C2 can bus (obd port pin 12 and 13)
		#pragma message("Building C2 BACCAble")

		#define ESC_TC_CUSTOMIZATOR_ENABLED // enable/disable ESC and Traction control (also controlled by C1 baccable) (by pressing LANE button (left stak) for 2 seconds it inverts current enabling status of ESC and TC features). it works only in race mode
		#define DYNO_MODE //disables all the controls (roll bench mode of MES) (also controlled by C1 baccable).
		#define FRONT_BRAKE_FORCER //disables front brakes (controlled by C1baccable).
		#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru C2baccable too
	#endif

	#if defined(BHbaccable) //this works only on BH can bus (obd port pin 3 and pin 11)
		#pragma message("Building BH BACCAble")

		#define SHOW_PARAMS_ON_DASHBOARD // used on new board to print text on dashboard (or if you connected together another baccable)
		#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru BHbaccable too
	#endif

	//note: with the following we avoid some combinations of defines.
	#if (defined(ACT_AS_CANABLE) && (defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)))
		#error "invalid combination of defines. If you want ACT_AS_CANABLE, disable C1, C2 and BH"
	#endif

	#if (defined(BHbaccable) &&		(defined(C1baccable) ||defined(C2baccable) ) )  //if required, let's automatically open the can bus
		#error "invalid combination of defines. If you want BH, disable C1 and C2"
	#endif

	#if ((defined(C2baccable)) &&	(defined(C1baccable)  || defined(BHbaccable)))
		#error "invalid combination of defines. If you want C2, disable C1 and BH"
	#endif

	#if (defined(SMART_DISABLE_START_STOP) && defined(DISABLE_START_STOP))
		#error "invalid combination of defines. Choose SMART_DISABLE_START_STOP or DISABLE_START_STOP."
	#endif




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

#if defined(ACT_AS_CANABLE)
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

		extern CAN_TxHeaderTypeDef uds_parameter_request_msg_header; //used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
		extern uint8_t baccableDashboardMenuVisible;
		extern uint8_t baccabledashboardMenuWasVisible; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
		extern uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
		extern uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
		extern uint16_t torque; //torque
		extern uint8_t batteryStateOfCharge; //battery charge %
		extern uint16_t batteryCurrent; //battery current (to be converted in Amps)
		extern uint8_t transmissionTemperature;
		extern uint8_t uds_parameter_request_msg_data[8];//used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
		extern uint8_t dashboardPageIndex; //to send message index - it changes when you press cruise control buttons - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality.
		extern uint32_t last_sent_uds_parameter_request_Time; //stores last time we send a uds parameter request - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality

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

		extern uint32_t lastReceivedCanMsgTime; //this is not in global C file, but comes from another C file

		//extern void sendDashboardPageToSlaveBaccable(float param); 	//this is defined in main.h
		//extern void sendMainDashboardPageToSlaveBaccable(); 		//this is defined in main.h
		//extern void sendSetupDashboardPageToSlaveBaccable(); 		//this is defined in main.h
		//extern void clearDashboardBaccableMenu();
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
		extern uint8_t DYNO_msg_data[5][6]; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno
		extern CAN_TxHeaderTypeDef DYNO_msg_header;

		extern uint32_t DynoStateMachineLastUpdateTime; //stores time (in milliseconds from power on) when Park Assist button press was read last time
		extern uint8_t ParkAssistButtonPressCount; //stores number of times this message field was received

		//FRONT_BRAKE_FORCER
		extern uint32_t last_sent_rear_brake_msg_time;

		extern CAN_TxHeaderTypeDef rearBrakeMsgHeader[4];
		extern uint8_t rearBrakeMsgData[4][8]; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(disable front brakes) (periodic)

	#endif

	#if defined(BHbaccable)
		extern uint32_t lastSentTelematic_display_info_msg_Time; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
		extern uint8_t telematic_display_info_field_totalFrameNumber; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
		extern uint8_t telematic_display_info_field_frameNumber; //current frame //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
		extern uint8_t telematic_display_info_field_infoCode; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
		extern uint8_t paramsStringCharIndex; // next char to send index - Used with SHOW_PARAMS_ON_DASHBOARD define functionality.
		extern CAN_TxHeaderTypeDef telematic_display_info_msg_header; //used when SHOW_PARAMS_ON_DASHBOARD is defined
		extern uint8_t telematic_display_info_msg_data[8]; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
		extern uint8_t requestToSendOneFrame; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard

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
		extern uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]; //used if SHOW_PARAMS_ON_DASHBOARD or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is declared - it contains string to print on dashboard

		extern float currentSpeed_km_h; //current vehicle speed

		extern uint32_t weCanSendAMessageReply; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
		extern uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
		extern uint32_t currentTime; //stores current time in milliseconds, each time we enter the main loop

		extern UART_HandleTypeDef huart2; // this is the serial line between baccables -- used with SHOW_PARAMS_ON_DASHBOARD and SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE and low consumption define functionalities

		extern uint32_t currentRpmSpeed;	//used by C1baccable
		extern uint8_t currentGear; 		//used by C1baccable

		// Storage for status and received message buffer
		extern CAN_RxHeaderTypeDef rx_msg_header;  //msg header
		extern uint8_t rx_msg_data[8];  //msg data

		extern uint8_t msg_buf[]; //msg converted in ascii to send over usb

		extern uint8_t _4wd_disabled; //if =4 disables 4wd
		extern uint8_t front_brake_forced; //if=5 disables Front brakes
		extern uint8_t commandsMenuEnabled; //if 0 disables the up-down buttons to change menu position

#endif /* INC_GLOBALVARIABLES_H_ */
