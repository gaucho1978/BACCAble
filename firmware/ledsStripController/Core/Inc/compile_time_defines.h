#ifndef INC_COMPILE_TIME_DEFINES_H_
#define INC_COMPILE_TIME_DEFINES_H_

	#ifdef INCLUDE_USER_CONFIG_H

	// define INCLUDE_USER_CONFIG_H globally (e.g. gcc -DINCLUDE_USER_CONFIG_H ...) or in the STM32CubeIDE project properties
	// under "C/C++ Build" -> "Settings" -> "MCU GCC Compiler" -> "Preprocessor"
	// to include the user_config.h header file at compile time
	// default: undefined
	// user can set preferred defines for its own custom builds; as - for example - it
	// can create a user_config.h (that won't be committed) setting DISABLE_DPF_REGEN_VISUAL_ALERT
	// and it will not get the dashboard warning when the DPF regeneration starts or LARGE_DISPLAY if it
	// owns a 7 inches dashboard etc. etc. (see below comments on defines)
	// user can still comment/uncomment defines, user_config.h is meant to be used to avoid conflicts
	// when pulling changes from remote
		#include "user_config.h"
	#endif

	//this is used to store FW version, also shown on usb when used as slcan
	#ifndef BUILD_VERSION //optional compile time define with -D, default: undefined
		#define BUILD_VERSION "V2.5.4"
	#endif
	#define _FW_VERSION "BACCAble " BUILD_VERSION


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

	//optional compile time defines for C1
	#ifdef C1baccable //this works only on C1 can bus (OBD port pins 6 and 14)
		#pragma message("Building C1 BACCAble") //adds a message in the compilation log

		//By default CLEAR_FAULTS_ENABLED functionality is active. can be deactivated by the user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_LOW_CONSUME
		#ifndef DISABLE_CLEAR_FAULTS_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("CLEAR_FAULTS_ENABLED functionality is active")
			#define CLEAR_FAULTS_ENABLED //baccable menu will allow to reset faults on all 3 baccable
		#endif

		//By default Low Consume functionality is enabled. can be disabled by the user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_LOW_CONSUME
		#ifndef DISABLE_LOW_CONSUME //optional compile time define with -D, default: undefined
			#pragma message("Low Consume is Enabled")
			#define LOW_CONSUME //master baccable will put other 2 chips and the other 2 can transceivers to sleep.
		#endif

		//By default BACCAble handle diesel params, can be changed by user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define IS_GASOLINE
		#ifndef IS_GASOLINE //optional compile time define with -D, default: undefined
			#pragma message("Will select default diesel engine parameters")
			#define IS_DIESEL
		#else
			#pragma message("Will select default gasoline engine parameters")
		#endif

		// UCAN_BOARD_LED_INVERSION is active by default, can be de activated by user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_UCAN_BOARD_LED_INVERSION
		#ifndef DISABLE_UCAN_BOARD_LED_INVERSION //optional compile time define with -D, default: undefined
			#pragma message("Ucan Board or New Baccable Board is used")
			#define UCAN_BOARD_LED_INVERSION //ucan fysect board (and  new BACCAble board) requires UCAN_BOARD_LED_INVERSION, since the led onboard are physically connected differently (status is inverted)
		#endif

		//Immobilizer is enabled by default, can be disabled by user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_IMMOBILIZER
		#ifndef DISABLE_IMMOBILIZER //optional compile time define with -D, default: undefined
			#pragma message("Enabling immobilizer")
			#define IMMOBILIZER_ENABLED
		#endif

		//start&stop disabler functionality is enabled by default, can be disabled by user, if required
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_THE_FUNCTION_SMART_DISABLE_START_STOP
		#ifndef DISABLE_THE_FUNCTION_SMART_DISABLE_START_STOP //optional compile time define with -D, default: undefined
			#pragma message("Enabling SMART_DISABLE_START_STOP functionality")
			#define SMART_DISABLE_START_STOP //start&stop will be automatically disabled with can message
		#endif


		// The following define should be moved in user_config if required. Left Here as reference
		// #define SHIFT_THRESHOLD 4500 //default shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		#ifndef SHIFT_THRESHOLD //optional compile time define with -D, default: undefined
			#pragma message("Setting Default Shift Threshold")
			#define SHIFT_THRESHOLD 4500 //default shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		#endif

		//currently this is no more used... it always tries to send message via serial line to slave baccable
		//I think we can remove this define. I should Better check the code before to do it.
		#define SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE //Used if you connected another baccable to usb port and want this baccable to send parameters to slave baccable (the slave will display parameter on the dashboard). if defined, the cruise control buttons + and - will change the shown parameter

		//Shift functionality is disabled by default. can be enabled by the user
		// The following define should be moved in user_config if required. Left Here as reference
		//#define SHIFT_INDICATOR_ENABLED //show shift indicator when rpm motor goes over the configurable threshold SHIFT_THRESHOLD (in race mode)
		#ifdef SHIFT_INDICATOR_ENABLED//optional compile time define with -D, default: undefined
			#pragma message("Enabling SHIFT INDICATOR")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define LED_STRIP_CONTROLLER_ENABLED
		#ifdef LED_STRIP_CONTROLLER_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling led strip controller")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define IPC_MY23_IS_INSTALLED //this is used in SHIFT_INDICATOR_ENABLED functionality, if you are using IPC for My23 Giulia/Stelvio
		#ifdef IPC_MY23_IS_INSTALLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling IPC_MY23_IS_INSTALLED")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define IPC_MY23_IS_INSTALLED //this is used in SHIFT_INDICATOR_ENABLED functionality, if you are using IPC for My23 Giulia/Stelvio
		#ifdef IPC_MY23_IS_INSTALLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling IPC_MY23_IS_INSTALLED")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define ROUTE_MSG // allows communication with  commercial diagnostic tools to supply internal bus parameters. better described in the manual
		#ifdef ROUTE_MSG //optional compile time define with -D, default: undefined
			#pragma message("Enabling ROUTE_MSG functionality")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define DYNO_MODE_MASTER //allows to request DYNO in master baccable
		#ifdef DYNO_MODE_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling DYNO_MODE_MASTER functionality on C1baccable")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define ESC_TC_CUSTOMIZATOR_MASTER //used to activate ESC/TC in master baccable
		#ifdef ESC_TC_CUSTOMIZATOR_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling ESC_TC_CUSTOMIZATOR_MASTER functionality on C1baccable")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		// #define ACC_VIRTUAL_PAD //simulate the presence of ACC button pad. When the user press button to enable/disable CC, the BACCABLE sends message to enable/disable ACC
		#ifdef ACC_VIRTUAL_PAD //optional compile time define with -D, default: undefined
			#pragma message("Enabling ACC_VIRTUAL_PAD functionality on C1baccable")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define FRONT_BRAKE_FORCER_MASTER //used to activate FRONT_BRAKE_FORCER in master baccable.
		#ifdef FRONT_BRAKE_FORCER_MASTER //optional compile time define with -D, default: undefined
			#pragma message("Enabling FRONT_BRAKE_FORCER_MASTER functionality on C1baccable")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define _4WD_DISABLER //disables 4WD
		#ifdef _4WD_DISABLER //optional compile time define with -D, default: undefined
			#pragma message("Enabling _4WD_DISABLER functionality on C1baccable")
		#endif

		//experimental: it doesn't work yet. don't use it!
		// The following define should be moved in user_config if required. Left Here as reference
		//#define REMOTE_START_ENABLED
		#ifdef REMOTE_START_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling remote start Functionality")
		#endif

		//experimental: it still do not work. don't use it!
		// The following define should be moved in user_config if required. Left Here as reference
		//#define READ_FAULTS_ENABLED // baccable menu will allow to read faults
		#ifdef READ_FAULTS_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling READ_FAULTS_ENABLED Functionality")
		#endif

		// The following define should be moved in user_config if required. Left Here as reference
		//#define REGENERATION_ALERT_ENABLED //if enabled, an alert wil be fired during each DPF regeneration process
		#ifdef REGENERATION_ALERT_ENABLED //optional compile time define with -D, default: undefined
			#pragma message("Enabling REGENERATION_ALERT_ENABLED Functionality")
		#endif

		//by default the DPF_REGEN_VISUAL_ALERT is enabled, but can be disabled if required by the user
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_DPF_REGEN_VISUAL_ALERT
		#ifndef DISABLE_DPF_REGEN_VISUAL_ALERT//optional compile time define with -D, default: undefined
			#pragma message("Enabling visual alert for DPF regeneration")
			#define DPF_REGEN_VISUAL_ALERT //BACCAble displays visual alert for DPF regeneration
		#endif

		//by default the DPF_REGEN_SOUND_ALERT is enabled, but can be disabled if required by the user
		// The following define should be moved in user_config if required. Left Here as reference
		//#define DISABLE_DPF_REGEN_SOUND_ALERT
		#ifndef DISABLE_DPF_REGEN_SOUND_ALERT//optional compile time define with -D, default: undefined
			#pragma message("Enabling sound alert for DPF regeneration")
			#define DPF_REGEN_SOUND_ALERT //BACCAble emits sound alert for DPF regeneration (belts alarm)
		#endif
	#endif


	#if defined(C2baccable) //this works only on C2 can bus (obd port pin 12 and 13)
		#pragma message("Building C2 BACCAble")

		//ToBeDone: make the following defines configurable in user_config, as done above

		#define ESC_TC_CUSTOMIZATOR_ENABLED // enable/disable ESC and Traction control (also controlled by C1 baccable) (by pressing LANE button (left stak) for 2 seconds it inverts current enabling status of ESC and TC features). it works only in race mode
		#define DYNO_MODE //disables all the controls (roll bench mode of MES) (also controlled by C1 baccable).
		#define FRONT_BRAKE_FORCER //disables front brakes (controlled by C1baccable).
		#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru C2baccable too
	#endif

	#if defined(BHbaccable) //this works only on BH can bus (obd port pin 3 and pin 11)
		#pragma message("Building BH BACCAble")

		//ToBeDone: make the following defines configurable in user_config, as done above

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




#endif
