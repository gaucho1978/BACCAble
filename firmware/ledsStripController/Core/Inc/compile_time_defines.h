#ifndef INC_COMPILE_TIME_DEFINES_H_
#define INC_COMPILE_TIME_DEFINES_H_

	//this is used to store FW version, also shown on usb when used as slcan
	#ifndef BUILD_VERSION //compile time define with -D
	#define BUILD_VERSION "V2.5.4"
	#endif
	#define FW_PREFIX "BACCABLE "
	#define _FW_VERSION FW_PREFIX BUILD_VERSION


	// force print
	#pragma message ("FW_VERSION: " _FW_VERSION)

	//WARNING: ACT_AS_CANABLE takes a lot of time to buffer and send packets to usb, therefore the main
	//         loop time duration increases. If you have C1baccable or C2baccable or BHbaccable, therefore can messages will
	//         start to be lost and some functions will not properly work. Then, if you use
	//         C1baccable, C2baccable or BHbaccable, it is recommended to comment ACT_AS_CANABLE.
	// 		   If the main loop duration increases too much, the device will not work properly and the
	//		   red led will start to continuously blink (you can find in the main loop, the loop duration check)


	//---------------------------------------------------------------------------------------------------------------------------
	// NEW
	//---------------------------------------------------------------------------------------------------------------------------

	//#define LARGE_DISPLAY //uncomment this to compile firmware for large displays; ideally you should use -D compiler args
	#ifndef LARGE_DISPLAY
		#define DASHBOARD_MESSAGE_MAX_LENGTH 18
	#else
		#define DASHBOARD_MESSAGE_MAX_LENGTH 24
	#endif

	// RELEASE_FLAVOR  is defined if compiling with eclipse
	#ifndef RELEASE_FLAVOR

		//If compiling with STM cube IDE, you will have to comment and uncomment these 4 lines:
		//#define ACT_AS_CANABLE //uncomment this to use the canable connected to the pc, as a usb-can adapter, for sniffing purposes
		#define C1baccable //uncomment this to compile firmware for baccable on C1 can bus
		//#define C2baccable //uncomment this to compile firmware for baccable on C2 can bus
		//#define BHbaccable //uncomment this to compile firmware for baccable on BH can bus
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

	// IS_GASOLINE  is defined if compiling with eclipse (on stm cube it will be not defined)
    #ifndef IS_GASOLINE
        #pragma message("Will select default diesel engine parameters")
        //If compiling with STM cube IDE, you will have to comment and uncomment the following line:
        #define IS_DIESEL //if uncommented sets by default diesel parameters (you can change it in setup menu), otherwise Gasoline will be default setting.
    #else
        #pragma message("Will select default gasoline engine parameters")
    #endif

    #ifndef DISABLE_UCAN_BOARD_LED_INVERSION
        #define UCAN_BOARD_LED_INVERSION //uncommented on ucan fysect board (and on new baccable board). the led onboard are physically connected differently (status is inverted)
    #endif
#endif
