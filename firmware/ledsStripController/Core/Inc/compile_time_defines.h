#ifndef INC_COMPILE_TIME_DEFINES_H_
#define INC_COMPILE_TIME_DEFINES_H_

	#ifdef INCLUDE_USER_CONFIG_H
// define this globally (e.g. gcc -DINCLUDE_USER_CONFIG_H ...) or in the STM32CubeIDE project properties
// under "C/C++ Build" -> "Settings" -> "MCU GCC Compiler" -> "Preprocessor"
// to include the user_config.h header file at compile time
// default: undefined
// user can set preferred defines for its own custom builds; as - for example - it
// can create a printf_config.h (that won't be committed) setting DISABLE_DPF_REGEN_VISUAL_ALERT
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

//optional compile time defines for C1
#ifdef C1baccable
	//By default BACCAble handle diesel params, can be changed by car user - dashboard menu
    #ifndef IS_GASOLINE//optional compile time define with -D, default: undefined
        #pragma message("Will select default diesel engine parameters")
        #define IS_DIESEL
    #else
        #pragma message("Will select default gasoline engine parameters")
    #endif

	//ucan fysect board (and on new BACCAble board). the led onboard are physically connected differently (status is inverted)
	#ifndef DISABLE_UCAN_BOARD_LED_INVERSION//optional compile time define with -D, default: undefined
        #define UCAN_BOARD_LED_INVERSION
    #endif

	//Immobilizer is enabled, can be disabled by car user - dedicated button
    #ifndef DISABLE_IMMOBILIZER//optional compile time define with -D, default: undefined
        #pragma message("Enabling immobilizer")
        #define IMMOBILIZER_ENABLED
    #endif

	//experimental: it doesn't work yet. don't use it!
    #ifdef ENABLE_REMOTE_START//optional compile time define with -D, default: undefined
        #pragma message("Enabling remote start immobilizer")
		#define REMOTE_START_ENABLED
    #endif

	#ifdef ENABLE_LED_STRIP_CONTROLLER//optional compile time define with -D, default: undefined
        #pragma message("Enabling led strip controller")
		#define LED_STRIP_CONTROLLER_ENABLED
    #endif

    //BACCAble displays visual alert for DPF regeneration
	#ifndef DISABLE_DPF_REGEN_VISUAL_ALERT//optional compile time define with -D, default: undefined
        #pragma message("Enabling visual alert for DPF regeneration")
        #define DPF_REGEN_VISUAL_ALERT
    #endif

    //BACCAble emits sound alert for DPF regeneration (belts alarm)
	#ifndef DISABLE_DPF_REGEN_SOUND_ALERT//optional compile time define with -D, default: undefined
        #pragma message("Enabling sound alert for DPF regeneration")
        #define DPF_REGEN_SOUND_ALERT
    #endif
#endif

#endif
