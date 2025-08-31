#ifndef INC_COMPILE_TIME_DEFINES_H_
#define INC_COMPILE_TIME_DEFINES_H_

	#ifdef INCLUDE_USER_CONFIG_H
		// In user_config.h file the user can set preferred defines for its own custom builds;
 		// Inside user_config.h.sample file you will find instructions about how to use it.
		#include <user_config.h>
	#endif

	//this is used to store FW version, also shown on usb when used as slcan
	#ifndef BUILD_VERSION //optional compile time define with -D, default: undefined
		#define BUILD_VERSION "V2.15.1"  //versioning rule: first digit major change, second digit minor change (like new feature), third digit bug fix or cosmetics
	#endif
	#define _FW_VERSION "BACCABLE " BUILD_VERSION


	// force print
	#pragma message ("FW_VERSION: " _FW_VERSION)

	//WARNING: ACT_AS_CANABLE takes a lot of time to buffer and send packets to usb, therefore the main
	//         loop time duration increases. If you have C1baccable or C2baccable or BHbaccable, therefore can messages will
	//         start to be lost and some functions will not properly work. Then, if you use
	//         C1baccable, C2baccable or BHbaccable, it is recommended to comment ACT_AS_CANABLE.
	// 		   If the main loop duration increases too much, the device will not work properly and the
	//		   red led will start to continuously blink (you can find in the main loop, the loop duration check)


	//---------------------------------------------------------------------------------------------------------------------------
	// RELEASE_FLAVOR  is defined if compiling with gcc or STM32CubeIde, default is ACT_AS_CANABLE
	#ifndef RELEASE_FLAVOR

		//If user's building with "Release" or "Debug" build configuration or on command line wth just "make"
		//then it will get this configuration; other defines are here just for reference (C1, C2 and BH)
		#define ACT_AS_CANABLE
		//#define C1baccable
		//#define C2baccable
		//#define BHbaccable
	#else
		#if RELEASE_FLAVOR == CAN_FLAVOR
			#define ACT_AS_CANABLE
		#elif RELEASE_FLAVOR == C1_FLAVOR
			#define C1baccable
		#elif RELEASE_FLAVOR == C2_FLAVOR
			#define C2baccable
		#elif RELEASE_FLAVOR == BH_FLAVOR
			#define BHbaccable
		#endif
	#endif

	//display size
	#ifndef LARGE_DISPLAY//optional compile time define with -D, default: undefined
		#define DASHBOARD_MESSAGE_MAX_LENGTH 18
	#else
		#define DASHBOARD_MESSAGE_MAX_LENGTH 24
	#endif

	//This section sets ACT_AS_CANABLE if no option was selected
	#if !defined(ACT_AS_CANABLE) && !defined(C1baccable) && !defined(C2baccable) && !defined(BHbaccable)
		#define ACT_AS_CANABLE
	#endif


	#ifdef ACT_AS_CANABLE
		#pragma message("Building CANable") //adds a message in the compilation log
	#endif

	#ifndef DISABLE_UCAN_BOARD_LED_INVERSION //optional compile time define with -D, default: undefined
		#pragma message("Ucan Board or New Baccable Board is used")
		#define UCAN_BOARD_LED_INVERSION //ucan fysect board (and  new BACCAble board) requires UCAN_BOARD_LED_INVERSION, since the led onboard are physically connected differently (status is inverted)
	#endif

	//optional compile time defines for C1
	#ifdef C1baccable //this works only on C1 can bus (OBD port pins 6 and 14)
		#pragma message("Building C1 BACCAble") //adds a message in the compilation log

		#ifndef DISABLE_CLEAR_FAULTS_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("CLEAR_FAULTS_ENABLED functionality is active")
			#define CLEAR_FAULTS_ENABLED //baccable menu will allow to reset faults on all 3 baccable
		#endif

		#ifndef DISABLE_LOW_CONSUME //optional compile time define with -D, default: undefined
			#pragma message("Low Consume is Enabled")
			#define LOW_CONSUME //master baccable will put other 2 chips and the other 2 can transceivers to sleep.
		#endif

		#ifndef IS_GASOLINE //optional compile time define with -D, default: undefined
			#pragma message("Will select default diesel engine parameters")
			#define IS_DIESEL
		#else
			#pragma message("Will select default gasoline engine parameters")
		#endif

		#ifdef PERMANENTLY_DISABLE_IMMO
			#pragma message("Permanently disabling immobilizer")
			#define DISABLE_IMMOBILIZER
		#endif

		#ifndef DISABLE_IMMOBILIZER //optional compile time define with -D, default: undefined
			#pragma message("Enabling immobilizer")
			#define IMMOBILIZER_ENABLED
		#endif

		#ifndef DISABLE_THE_FUNCTION_SMART_DISABLE_START_STOP //optional compile time define with -D, default: undefined
			#pragma message("Enabling SMART_DISABLE_START_STOP functionality")
			#define SMART_DISABLE_START_STOP //start&stop will be automatically disabled with can message
		#endif

		#ifndef SHIFT_THRESHOLD //optional compile time define with -D, default: undefined
			#pragma message("Setting Default Shift Threshold")
			#define SHIFT_THRESHOLD 4500 //default shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		#endif

		#ifdef SHIFT_INDICATOR_ENABLED//optional compile time define with -D, default: undefined
			#pragma message("Enabling SHIFT INDICATOR")
		#endif

		#ifdef LED_STRIP_CONTROLLER_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling led strip controller")
		#endif

		#ifdef IPC_MY23_IS_INSTALLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling IPC_MY23_IS_INSTALLED")
		#endif

		#ifdef ROUTE_MSG //optional compile time define with -D, default: undefined
			#pragma message("Enabling ROUTE_MSG functionality")
		#endif

		#ifdef DYNO_MODE_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling DYNO_MODE_MASTER functionality on C1baccable")
		#endif

		#ifdef ESC_TC_CUSTOMIZATOR_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling ESC_TC_CUSTOMIZATOR_MASTER functionality on C1baccable")
		#endif

		#ifdef ACC_VIRTUAL_PAD //optional compile time define with -D, default: undefined
			#pragma message("Enabling ACC_VIRTUAL_PAD functionality on C1baccable")
		#endif

		#ifdef FRONT_BRAKE_FORCER_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling FRONT_BRAKE_FORCER_MASTER functionality on C1baccable")
		#endif

		#ifdef _4WD_DISABLER //optional compile time define with -D, default: undefined
			#pragma message("Enabling _4WD_DISABLER functionality on C1baccable")
		#endif

		#ifdef REMOTE_START_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling remote start Functionality")
		#endif

		#ifdef READ_FAULTS_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling READ_FAULTS_ENABLED Functionality")
		#endif

		#ifdef REGENERATION_ALERT_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling REGENERATION_ALERT_ENABLED Functionality")
		#endif

		#ifndef DISABLE_DPF_REGEN_VISUAL_ALERT//optional compile time define with -D, default: undefined
			#pragma message("Enabling visual alert for DPF regeneration")
			#define DPF_REGEN_VISUAL_ALERT //BACCAble displays visual alert for DPF regeneration
		#endif

		#ifndef DISABLE_DPF_REGEN_SOUND_ALERT//optional compile time define with -D, default: undefined
			#pragma message("Enabling sound alert for DPF regeneration")
			#define DPF_REGEN_SOUND_ALERT //BACCAble emits sound alert for DPF regeneration (belts alarm)
		#endif

		#ifdef SEATBELT_ALARM_DISABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling SEATBELT_ALARM_DISABLED Functionality")
		#endif

		#ifdef TORQUE_CORRECTION_FACTOR //optional compile time define with -D, default: undefined
			#pragma message("Enabling TORQUE_CORRECTION_FACTOR Correction")
		#endif

		#ifdef PEDAL_BOOSTER_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling PEDAL_BOOSTER_ENABLED Functionality")
		#endif

		#ifdef DISABLE_ODOMETER_BLINK //optional compile time define with -D, default: undefined
			#pragma message("Enabling function to disable odometer blink")
		#endif

		#ifdef SHOW_RACE_MASK //optional compile time define with -D, default: undefined
			#pragma message("Enabling function to show race mask when ESC/TC is activated")
		#endif


		#ifdef HIDE_DASHBOARD_MENU
			#pragma message("dashboard menu will be permanently hidden")
		#endif

		#ifdef ACC_AUTOSTART
			#pragma message("Enabling function ACC Autostart")
		#endif

		#ifdef CLOSE_WINDOWS
			#pragma message("Enabling function to Close Windows when door is locked")
		#endif


	#endif


	#if defined(C2baccable) //this works only on C2 can bus (obd port pin 12 and 13)
		#pragma message("Building C2 BACCAble")

		#ifndef DISABLE_ESC_TC_CUSTOMIZATOR //optional compile time define with -D, default: undefined
			#pragma message("Enabling ESC_TC_CUSTOMIZATOR functionality")
			#define ESC_TC_CUSTOMIZATOR_ENABLED // enable/disable ESC and Traction control (also controlled by C1 baccable) (by pressing LANE button (left stak) for 2 seconds it inverts current enabling status of ESC and TC features). it works only in race mode
		#endif

		#ifndef DISABLE_DYNO_MODE //optional compile time define with -D, default: undefined
			#pragma message("Enabling DYNO_MODE functionality")
			#define DYNO_MODE //disables all the controls (roll bench mode of MES) (also controlled by C1 baccable).
		#endif

		#ifndef DISABLE_FRONT_BRAKE_FORCER //optional compile time define with -D, default: undefined
			#pragma message("Enabling FRONT_BRAKE_FORCER functionality")
			#define FRONT_BRAKE_FORCER //disables front brakes (controlled by C1baccable).
		#endif

		#ifndef DISABLE_CLEAR_FAULTS_C2 //optional compile time define with -D, default: undefined
			#pragma message("Enabling CLEAR_FAULTS functionality")
			#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru C2baccable too
		#endif

	#endif

	#if defined(BHbaccable) //this works only on BH can bus (obd port pin 3 and pin 11)
		#pragma message("Building BH BACCAble")

		#ifndef DISPLAY_INFO_CODE//optional compile time define with -D, default: undefined
			#define DISPLAY_INFO_CODE 0x09
		#endif
		#ifndef DISABLE_CLEAR_FAULTS_BH //optional compile time define with -D, default: undefined
			#pragma message("Enabling CLEAR_FAULTS functionality")
			#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru BHbaccable too
		#endif

		#ifdef DISABLE_ODOMETER_BLINK //optional compile time define with -D, default: undefined
			#pragma message("Enabling function to disable odometer blink")
		#endif

	#endif

	//note: with the following we avoid some combinations of defines.
	#if (defined(DEBUG_MODE) && (defined(ACT_AS_CANABLE) || defined(LED_STRIP_CONTROLLER_ENABLED)))
		#error "invalid combination of defines. If you want DEBUG_MODE, disable ACT_AS_CANABLE, LED_STRIP_CONTROLLER_ENABLED"
	#endif

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




#endif
