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





	#define currentTime HAL_GetTick()

	#define ENGINE_NORMAL				0
	#define ENGINE_DPF_REGEN_LO			1
	#define ENGINE_DPF_REGEN_HI			2
	#define ENGINE_NSC_DE_NOX_REGEN		3
	#define ENGINE_NSC_DE_SOX_REGEN		4
	#define ENGINE_SCR_HEATUP_STRATEGY	5

	#define UART_BUFFER_SIZE DASHBOARD_MESSAGE_MAX_LENGTH + 1

	#define UART1_BUFFER_SIZE 9  //legth for schizzaforte messages


	#define LAST_PAGE_ADDRESS (FLASH_BANK1_END - FLASH_PAGE_SIZE +1) // 0x0801F800 //valid only for stm32F072 i suppose
			//la flash inizia a 0x08000000  e finisce a 0x0801FFFF, -0x800 +1 di una pagina fa 0x0801F800
	#define LAST_PAGE_ADDRESS_STATISTICS LAST_PAGE_ADDRESS- FLASH_PAGE_SIZE //we will use previous page for statistics
	#define LAST_PAGE_ADDRESS_SHOWN_PARAMS LAST_PAGE_ADDRESS_STATISTICS- FLASH_PAGE_SIZE //we will use previous page for shown params

	#define TIMING__ALL___SERIAL_IGNORE_WINDOW_MS										2000	//msec
	#define TIMING__C2_BH_USB_CONNECT_TO_C1_NOTIFICATION_DELAY_MS						TIMING__ALL___SERIAL_IGNORE_WINDOW_MS+100	//msec
	#define TIMING__C1____DELAY_BEFORE_SERIAL_PROCESS_AFTER_OTHER_CHIP_WAKE_MS			TIMING__ALL___SERIAL_IGNORE_WINDOW_MS+100	//msec
	#define TIMING__C1____DELAY_BEFORE_OTHER_CHIP_STATUS_REQUEST_MS						TIMING__ALL___SERIAL_IGNORE_WINDOW_MS+1000	//msec

	#define TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS						3500	//msec
	#define TIMING__C1____CAN_ACTIVITY_WINDOW_FOR_WAKEUP_MS								TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS-100	//msec
	#define TIMING__C1____DELAY_BEFORE_SERIAL_INSTRUCT_OF_C2BH_AFTER_OTHER_CHIP_WAKE_MS	TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS+2	//msec


	#define TIMING__C1____C2_STATUS_REQUEST_TIMEOUT_MS									1010	//msec
	#define TIMING__C1____BH_STATUS_REQUEST_TIMEOUT_MS									TIMING__C1____C2_STATUS_REQUEST_TIMEOUT_MS+250	//msec

	#define TIMING__C1____SERIAL_TIMEOUT_REPLY_MS										250		//msec
	#define TIMING__C2_BH_SERIAL_TIMEOUT_REPLY_MS										TIMING__C1____SERIAL_TIMEOUT_REPLY_MS-50		//msec


	#define TIMING__C1____SCHIZZAFORTE_SERIAL_TIMEOUT_REPLY_MS							100		//msec

	#define TIMING__BH____PAUSE_BETWEEN_PARK_MIRROR_COMMANDS							2500	//msec

	//park mirror - Neutral (currentGear==0x00) shorter than this is a mere gear-shift transient (R->D, D->N->D, ...)
	#define TIMING__BH____NEUTRAL_GEAR_TRANSIENT_MS									500		//msec

	//park sensors mute - hold time between the simulated push and release of the button (message 0x5B0)
	#define TIMING__C2____PDC_BUTTON_PRESS_MS											50		//msec

#if defined(ACT_AS_CANABLE) ||  defined(DEBUG_MODE) || defined(ENABLE_USB_MASS_STORAGE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER) || defined(C1baccable) //sniffer function 24/08/2026 - C1 added: sniffer needs the usb cdc symbols
	//#include "usbd_def.h"
	#include "usb_device.h"
	#include "usbd_desc.h" //sniffer function 24/08/2026 - usbdDescSelectCdcMode() and usbdDescCdcModeSelected

	#ifdef ENABLE_USB_MASS_STORAGE
		#include "usbd_storage_if.h"
		#include "ff.h"
		#include "usbd_cdc_if.h" //sniffer function 24/08/2026 - sniffer transmits over cdc even on mass storage builds
	#else
		#include "string.h"
		#include "usbd_cdc_if.h"
	#endif

#endif


	#include "onboardLed.h"
	#include "can.h"
	#include "slcan.h"
	#include "math.h"


	#include "uart.h"

	#if defined(C1baccable)
		#include "vuMeter.h" //this is used to control led strip through usb pin
		#include "lowConsume.h"
		#include "uds_parameters.h"
		#include "elm327.h" //elm327 function 26/08/2026
		//extern uint32_t lastReceivedCanMsgTime;
	#endif





		//all variables of globalVariables.c shall be repeated here as extern
		extern const char *FW_VERSION;
		extern const uint8_t led_light_on_bit;



	#if defined(C1baccable) || defined (ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
		extern UART_HandleTypeDef huart1; // this is the serial line toward schizzaforte
		extern uint8_t currentSchizzaforteMap;
		extern int8_t pedal_map_power; //pedal map amplification, between -10 and +10
		extern uint8_t pedal_map_power_adapted; //adaptation depending on current map
		extern uint32_t last_queued_serial_to_schizzaForte_msg_time;
		extern uint8_t playMotorJingle; //position of the execution of the jingle. set to 255 to start execution
		extern uint8_t jingleArray[255];

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

		extern uint8_t main_dashboardPageIndex;
		extern uint8_t dashboard_menu_indent_level;
		extern uint8_t maxHold_enabled;
		extern uint8_t dashboard_main_menu_array_len;
		extern uint8_t dashboard_main_menu_array[20][DASHBOARD_MESSAGE_MAX_LENGTH];
		extern uint8_t setup_dashboardPageIndex;
		extern uint8_t total_pages_in_setup_dashboard_menu;
		extern uint8_t dashboard_setup_menu_array[31][DASHBOARD_MESSAGE_MAX_LENGTH]; //elm327 function 26/08/2026 - was [30]

		//extern uint8_t params_setup_dashboardPageIndex;
		//extern uint8_t shownParamsArray[240];
		//extern uint8_t total_pages_in_params_setup_dashboard_menu;

		extern uint8_t function_is_diesel_enabled; //stored in flash. defines if we use gasoline (0) or diesel (1) params
		//extern uint8_t total_pages_in_dashboard_menu_diesel;
		//extern uint8_t total_pages_in_dashboard_menu_gasoline;
		// uds_params_array[0] contais gasoline params, , uds_params_array[1] contains diesel params
		//extern const	uds_param_element uds_params_array[2][60]; // initializes all the uds parameters request to send and reply to receive

		extern CAN_TxHeaderTypeDef uds_parameter_request_msg_header;
		extern uint8_t baccableDashboardMenuVisible;
		extern uint8_t baccabledashboardMenuWasVisible; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
		extern uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
		extern uint8_t oilLevel; //oil level from 0 to 14
		extern uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
		extern uint8_t waterTemperature; //water temperature in celsius degrees (to correct by offset)
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
		extern CAN_TxHeaderTypeDef REMOTE_START_msg_header;
		extern uint8_t REMOTE_START_msg_data[8];
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
		//extern uint8_t STATUS_ECM_msg_data[8];
		//extern CAN_TxHeaderTypeDef STATUS_ECM_msg_header;
		extern uint8_t loopsFromRegenerationEnded;

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


		//FRONT_BRAKE_FORCER_MASTER
		extern uint8_t function_front_brake_forcer_master; //stored in flash
		extern uint16_t launch_torque_threshold; //stored in flash

		//extern uint32_t lastReceivedCanMsgTime; //this is not in global C file, but comes from another C file

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

		//DISABLE_ODOMETER_BLINK
		extern uint8_t function_disable_odometer_blink;

		extern uint8_t instructSlaveBoardsTriggerEnabled;

		//ACC Autostart
		extern uint8_t function_acc_autostart;
		extern uint32_t lastSentAutostartMsg;
		extern uint8_t AutostartMsgCounter;
		extern uint8_t brakeIntervention_ACC_ESC_ASR;

		//CLOSE_WINDOWS
		extern uint8_t function_close_windows_with_door_lock;
		extern uint8_t closeWindowsRequest;
		extern uint32_t doorCloseTime;
		extern uint8_t doorLocksRequestsCounter;

		extern uint8_t function_open_windows_with_door_lock;
		extern uint8_t openWindowsRequest;
		extern uint32_t doorOpenTime;
		extern uint8_t doorUnlocksRequestsCounter;

		extern uint8_t RF_requestor;
		extern uint8_t RF_fob_number;

		//QV_EXHAUST_FLAP_FUNCTION_ENABLED
		extern uint8_t QV_exhaust_flap_function_enabled;
		extern uint8_t ForceQVexhaustValveOpened;
		extern uint32_t lastSentQVexhaustValveMsgTime;
		extern CAN_TxHeaderTypeDef forceQVexhaustValveMsgHeader[4];
		extern uint8_t forceQVexhaustValveMsgData[4][8];
		extern uint8_t numberOfReleaseButtonClicks;
		extern uint32_t ReleasebuttonFirstClickTime;
		extern uint32_t ReleasebuttonPressBeginTime;

		//variables for chinese valves management (radiocontrol buttons pressed by baccable C1)
		extern uint32_t exhaustValveMosfetCommandTime; //time when mosfet was closed
		extern uint8_t ChineseExhaustValveRequest; //0=no request, 'O'=open request, 'C'=Close request
		extern uint8_t chineseValveIsOpened; //0=closed, 1=opened

		//pumpForce test25/07/2026 - BEGIN
		// Variabili per la sequenza UDS di forzatura pompa carburante verso ECM (ECU 0x10, ExtId tx 0x18DA10F1)
		// Macchina a stati pumpForceStateMachine:
		//   0xFF = inattivo  |  0 = attende conf. sessione (50 03)  |  1 = attende seed (67 01)
		//   2 = attende acc. sicurezza (67 02)  |  3 = invio periodico IO Control ogni 200ms
		extern uint8_t  function_pump_force_enabled;   // flag abilitazione: impostare =1 per avviare dopo 10s dal boot
		extern uint8_t  pumpForceStateMachine;          // stato corrente della macchina a stati (init 0xFF = inattivo)
		extern uint8_t  ecmSeed[4];                    // seed 4 byte big-endian ricevuto dalla ECM (risposta 67 01)
		extern uint32_t lastPumpForceMsgTime;           // timestamp ultimo msg inviato: usato per timeout (stati 0/1/2) e timer 200ms (stato 3)
		extern CAN_TxHeaderTypeDef pumpForceTxHeader;  // header CAN dedicato (ExtId=0x18DA10F1, IDE=EXT, DLC aggiornato prima di ogni tx)
		extern uint8_t  pumpForceTxData[8];            // buffer dati CAN riusato per tutti i messaggi della sequenza
		//pumpForce test25/07/2026 - END

		//readFaults 12/08/2026 - BEGIN		// Variabili per la sequenza UDS di lettura DTC dal Body ECU (ECU 0x40, ExtId tx 0x18DA40F1)
		// faultsStateMachine:
		//   0xFF=inattivo | 0=attende conf.sessione (50 03) | 1=attende ReadDTC resp
		//   2=ricezione multiframe in corso | 3=lista DTC pronta | 4=TIMEOUT display
		#define FAULTS_DTC_MAX 20
		extern uint8_t  faultsStateMachine;
		extern uint8_t  faultsDTCcount;
		extern uint8_t  faultsDTCsubmenuIndex;
		extern uint8_t  faultsDTCbytes[FAULTS_DTC_MAX][3];
		extern uint8_t  faultsRxBuffer[90];
		extern uint16_t faultsRxExpected;
		extern uint16_t faultsRxReceived;
		extern uint8_t  faultsRxNextSN;
		extern uint32_t faultsTimer;
		extern CAN_TxHeaderTypeDef faultsBodyTxHeader;
		extern uint8_t  faultsBodyTxData[8];
		//readFaults 12/08/2026 - END

	#endif

	#if defined (C1baccable) || defined (C2baccable)
		//function animation lights
		extern uint8_t function_lights_animation_enabled;
		extern uint8_t lights_animation_state_machine;
		extern uint8_t parkSensorsMuteFunctionEnabled;

		//
		extern uint8_t carSteadyCounter; //how long the car has been steady, capped at 200 (=2000msec). Fed on C1 only, from 0x101

	#endif

	#if defined(C2baccable)

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

		extern uint8_t reverseGearActive;
		extern uint8_t parkSensorsFunctionStatus; //0=park sensors off, anything else=on
		extern uint8_t parkSensorsLedStatus; //0=off, 1=continuous, 2=blink

		// @netzmark PDC auto disable - the front chime is silenced by simulating a press of the park sensors
		// button (0x5B0) when the brake is pressed firmly while the sensors are beeping. See functions_C2baccable.c
		extern volatile uint8_t  pdc_is_beeping;		//1=front sensors in alarm (from 0x3E7)
		extern volatile uint8_t  pdc_auto_disabled;		//1=the park sensors are off because WE switched them off
		extern volatile uint8_t  requestToTogglePDC;	//1=a button press has to be simulated
		extern volatile int      pdc_send_counter;		//0=nothing in progress, 1=button pushed, waiting to release it
		extern volatile uint32_t last_pdc_shot_time;	//when the push was sent
		extern uint8_t pdcMsgData[8];
		extern CAN_TxHeaderTypeDef pdcMsgHeader;
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
		extern uint8_t disable_odometer_blink;

	#endif

	#if defined(C2baccable) || defined(BHbaccable)
		extern FATFS fs; //filesystem
	#endif

	//ESC_TC_CUSTOMIZATOR_MASTER)
	extern uint8_t function_esc_tc_customizator_enabled; //stored in flash

	//ESC_TC_CUSTOMIZATOR_ENABLED
	//extern uint32_t LANEbuttonPressLastTimeSeen; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
	//extern uint32_t LANEbutton2PressLastTimeSeen; //like previous one but for lane button on my20 cars. Stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
	extern uint32_t LANEbuttonPressBeginTime;
	extern uint32_t LANEbuttonFirstClickTime;
	//extern uint8_t LANEbuttonPressCount; //stores number of times this message field was received
	//extern uint8_t LANEbutton2PressCount; //like previous one but for lane button on my20 cars. Stores number of times this message field was received
	extern uint8_t numberOfLaneButtonClicks; //number of times button was pressed(click)

	//PEDAL_BOOSTER_ENABLED
	extern uint8_t function_pedal_booster_enabled;

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
	extern uint32_t currentTimeMainLoopDebug; //stores current time in milliseconds, each time we enter the main loop

	extern UART_HandleTypeDef huart2; // this is the serial line between baccables

	extern uint32_t currentRpmSpeed;	//used by C1baccable
	extern uint8_t currentGear; 		//used by C1baccable and BHbaccable

	// Storage for status and received message buffer
	extern CAN_RxHeaderTypeDef rx_msg_header;  //msg header
	extern uint8_t rx_msg_data[8];  //msg data

	extern uint8_t msg_buf[]; //msg converted in ascii to send over usb

	extern uint8_t _4wd_disabled; //if =4 disables 4wd
	extern uint8_t front_brake_forced; //if=5 disables Front brakes
	extern uint8_t DynoModeEnabledOnMaster; //status of dyno in master board. tells if dyno is active
	extern uint32_t last_4wd_disabled_overlay_time; // 4wd constraint relax change 24/08/2026
	extern uint8_t  show_4wd_disabled_overlay;       // 4wd constraint relax change 24/08/2026

	//sniffer function 24/08/2026 - BEGIN
	//raw can frames are streamed to usb cdc with a fixed 16 byte layout:
	//  byte 0     : 0xA0 | DLC          (0xA = start nibble, DLC 0..8; 0xAF = frames lost marker)
	//  byte 1..3  : timestamp, 24 bit little endian (currentTime, ms)
	//  byte 4..7  : can id, 32 bit little endian
	//  byte 8..15 : payload, zero padded above DLC
	//gated to C1/C2/BH only: ACT_AS_CANABLE has no menu to trigger the function and does not need the ram
	#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
		#define SNIFFER_FRAME_SIZE					16		//bytes per frame
		#define SNIFFER_FRAME_COUNT					16		//frames kept in ram
		#define SNIFFER_BUFFER_SIZE					(SNIFFER_FRAME_SIZE*SNIFFER_FRAME_COUNT) //256 bytes, power of two: index wrap done with a mask
		#define SNIFFER_BUFFER_MASK					(SNIFFER_BUFFER_SIZE-1)
		#define SNIFFER_USB_CHUNK					64		//usb cdc linear tx buffer size (TX_BUF_SIZE), holds exactly 4 frames
		#define SNIFFER_FLUSH_TIMEOUT_MS			20		//flush a partial buffer after this idle time, to keep latency low on a quiet bus
		#define SNIFFER_START_NIBBLE				0xA0	//high nibble marking the first byte of a frame
		#define SNIFFER_OVERFLOW_MARKER				0xAF	//invalid DLC 15: frame carrying the number of lost frames
		#define SNIFFER_CAN_FRAMES_PER_LOOP			3		//bxCAN RX FIFO0 depth on stm32F072: never more than this pending
		//Also doubles as the disconnect timeout once the host has been seen: snifferActivationTime is refreshed on
		//every loop while the host keeps us CONFIGURED, so this is really "give up after this long without a host".
		//Sized on the slowest link in the chain: a slave (C2/BH) is only told to start when C1 sends the enable
		//message, then its own confirmation back to C1 has to wait for C1's next status poll (up to ~1260ms, see
		//uart.c), and only then is C1's sleep guard latched. 10s leaves a comfortable margin for android usb
		//enumeration on top of that polling latency. //sniffer function 24/08/2026
		#define SNIFFER_ACTIVATION_TIMEOUT_MS		10000	//if the host has not configured us within this time from activation, usb is fully powered down and we go back to being a plain baccable

		extern uint8_t  snifferInUse;				//0=off, 1=on while the function is starting up or actively running. never saved on flash by itself
		extern uint8_t  snifferFunctionEnabled;		//0=off, 1=on. saved on flash (slot 31), toggled from the setup menu, applied on SAVE&EXIT and at boot/wake
		extern uint32_t snifferActivationTime;		//rolling "last time a host was seen": set when the attempt starts, then refreshed on every loop while the host keeps us CONFIGURED. drives the SNIFFER_ACTIVATION_TIMEOUT_MS give up window
		extern uint8_t  snifferActivationConfirmed;	//1 once the host has configured us during the current activation attempt: the timeout is only checked before this latches
		extern uint8_t  snifferUsbInited;			//1 once the usb has been (re)started as cdc for the sniffer. one shot, no retry: stays 1 even if the host never configures us
		extern uint8_t  snifferUsbStartRequested;	//set when the function is turned on, served by the main loop (usb bring up blocks for some ms)
		extern uint8_t  snifferUsbShutdownRequested;	//set when the function is turned off, served by the main loop (usb shutdown blocks for some ms)
		extern uint8_t  snifferRingBuffer[SNIFFER_BUFFER_SIZE];
		extern uint16_t snifferRingHead;			//write index
		extern uint16_t snifferRingTail;			//read index
		extern uint16_t snifferRingCount;			//bytes currently stored
		extern uint16_t snifferDroppedFrames;		//frames lost since last overflow marker
		extern uint32_t snifferLastFlushTime;
		extern USBD_HandleTypeDef hUsbDeviceFS;	//declared in usb_device.c, needed to check TxState before a non blocking send

		#ifdef DEBUG_CAN_RX_SIMULATION
			extern uint32_t debugSimulatedMsgLastInjectTime;
			extern uint8_t debugSimulatedMsgByteCounter;
		#endif
		#ifdef DEBUG_START_SNIFFER //sniffer function 24/08/2026 - bench test with no vehicle connected
			extern uint8_t  debugSnifferAutoStarted;	//0 until the 10 second auto-start has fired once
		#endif

		#ifdef DEBUG_START_ELM327 // bench test with no vehicle connected
			extern uint8_t  debugElm327AutoStarted;	//0 until the 10 second auto-start has fired once
		#endif
	#endif

	//elm327 function 26/08/2026 - BEGIN
	//Same three state lifecycle as the sniffer above, with one crucial difference: the sniffer works alongside
	//the baccable, ELM327 replaces it (C1 stops decoding can and hands the inter chip serial line over to the
	//diagnostic bridge, so the dashboard stops responding). So the suspension is tied to ActivationConfirmed,
	//NOT to InUse: while we are only waiting for a host the baccable keeps working exactly as always, and if
	//no host ever shows up we simply switch the usb back off and nobody notices.
	//   elm327FunctionEnabled : preference, saved on flash (slot 32), toggled from the setup menu
	//   elm327InUse           : usb is up and we are waiting for (or talking to) a host
	//   elm327ActivationConfirmed : a host really enumerated us -> the interpreter is actually running
	#if defined(C1baccable)
		extern uint8_t  elm327FunctionEnabled;
		extern uint8_t  elm327InUse;
		extern uint32_t elm327ActivationTime;		//rolling "last time a host was seen", same meaning as snifferActivationTime
		extern uint8_t  elm327ActivationConfirmed;
		extern uint8_t  elm327UsbStartRequested;	//served by the main loop: bringing usb up blocks for some ms
	#endif
	//elm327 function 26/08/2026 - END
	//sniffer function 24/08/2026 - END

	extern uint8_t launch_assist_enabled; //if=1 assist is enabled and uses torque as trigget to release front brakes

	extern uint8_t commandsMenuEnabled; //if 0 disables the up-down buttons to change menu position

	//LOW CONSUME
	extern uint8_t lowConsumeIsActive; //0=false, 1=true
	extern uint32_t lastReceivedCanMsgTime;
	extern uint32_t lastTrigger; //temporary debug variable
	extern uint8_t tmpCounter;

	extern uint32_t allProcessorsWakeupTime;

	//ESC/TC function (common to C1,C2,BH
	extern uint8_t currentDNAmode; //0x00=Natural, 0x08=dynamic 0x10=AllWeather, 0x30=race
	extern uint8_t DNA_msg_data[8];
	extern CAN_TxHeaderTypeDef DNA_msg_header;
	extern uint32_t lastSent384;

	extern uint8_t ESCandTCinversion; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)

	//SHOW RACE MASK
	extern uint8_t function_show_race_mask;

	//PARK_MIRROR
	extern uint8_t function_park_mirror;
	extern uint8_t leftMirrorHorizontalOperativePos;	//current Operative position
	extern uint8_t leftMirrorVerticalOperativePos; 	//current Operative position
	extern uint8_t rightMirrorHorizontalOperativePos;//current Operative position
	extern uint8_t rightMirrorVerticalOperativePos;	//current Operative position
	extern uint8_t storeOperativeMirrorPosition;//get Operative Operative position boolean
	extern uint8_t leftParkMirrorVerticalPos; //Stored Park position
	extern uint8_t leftParkMirrorHorizontalPos; //Stored Park position
	extern uint8_t rightParkMirrorVerticalPos;	//Stored Park position
	extern uint8_t rightParkMirrorHorizontalPos; //Stored Park position
	extern uint8_t storeCurrentParkMirrorPosition; //Store Park mirrors position Request
	extern uint8_t parkMirrorsSteady; // park mirrors are not moving if this is =1
	extern uint8_t turnIndicator; //0= center, 1=right, 2=left
	extern uint8_t parkMirrorMsgData[8];
	extern CAN_TxHeaderTypeDef parkMirrorMsgHeader;
	extern uint32_t lastParkMirrorMsgTime;
	extern uint32_t restoreOperativeMirrorsPositionRequestTime;
	extern uint8_t restoreOperativeMirrorsPosition;
	extern uint32_t leftParkMirrorPositionRequiredRequestTime;
	extern uint8_t leftParkMirrorPositionRequired;
	extern uint32_t rightParkMirrorPositionRequiredRequestTime;
	extern uint8_t rightParkMirrorPositionRequired;
	extern uint8_t parkMirrorOperativePositionNotStored;
	extern uint32_t exitReverseTime; //@netzmark parkingMirror returning delay
	extern uint32_t neutralGearEntryTime; //park mirror - moment Neutral (currentGear==0x00) was entered, 0 when not in Neutral

	//HAS_FUNCTION_ENABLED
	extern uint8_t HAS_function_enabled;
	extern uint8_t HAS_buttonPressRequested;

	extern uint8_t neverSaved;
	extern uint8_t usbInited;

	extern uint32_t lastUartErrorCallback;

	extern uint8_t usbConnectedToSlave;
	//sniffer function 24/08/2026 - on C1 only: tracked per slave, because usbConnectedToSlave above cannot tell
	//WHICH slave reported. usbConnectedToSlave is kept as the OR of these two, so lowConsume.c needs no change.
	extern uint8_t usbConnectedToC2;
	extern uint8_t usbConnectedToBH;




#endif /* INC_GLOBALVARIABLES_H_ */
