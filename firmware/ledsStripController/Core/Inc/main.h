#ifndef __MAIN_H
#define __MAIN_H

	#ifdef __cplusplus
		extern "C" {
	#endif

		//#define ACT_AS_CANABLE //uncomment this to use the canable connected to the pc, as a usb-can adapter, for sniffing purposes
		#define DISABLE_START_STOP //comment this if you don't want to automatically disable start&stop car functionality
		#define IMMOBILIZER_ENABLED //this works only on C1 can bus (OBD port pins 6 and 14) //comment this if you don't want immobilizer functionality. This functionality waits for a thief to connect to RFHUB and if the connection message is found, it resets the rfhub and it starts the Panic alarm.
		#define LED_STRIP_CONTROLLER_ENABLED //this was tested only on C1 can bus (OBD port pins 6 and 14) //comment this if you don't want led strip controller functionality //NOTE: it can not be used with USE_AS_CANABLE cause both uses usb port
		#define SHIFT_INDICATOR_ENABLED //this was tested only on C1 can bus (OBD port pins 6 and 14) //comment this line if you don't want to show shift indicator when rpm motor goes over the configurable threshold SHIFT_THRESHOLD (in race mode)
		#define SHIFT_THRESHOLD 2000 //this is the configurable shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		//#define ESC_TC_CUSTOMIZATOR_ENABLED //this works only on C2 can bus (obd port pin 12 and 13) //--// uncomment this line if you want to be able to enable/disable ESC and Traction control (pressing LANE button (left stak) for 2 seconds it inverts current status of ESC and TC features, so if they are enabled, they will be disabled and viceversa)
		//#define SHOW_PARAMS_ON_DASHBOARD //this works only on BH can bus (obd port pin 3 and pin 11) //--// uncomment this if you want to show on dashboard some parameters (using up and down buttons on cruise control panel, on the steering wheel)

		//WARNING: ACT_AS_CANABLE takes a lot of time to buffer and send packets to usb, therefore the main
		//         loop time duration increases. If you have IMMOBILIZER_ENABLED therefore can messages will
		//         start to be lost and IMMOBILIZER function will not properly work. Then, if you use
		//         IMMOBILIZER_ENABLED, it is recommended to comment ACT_AS_CANABLE. Anyway you can test it
		//         by your own, by checking main loop time duration with both functions enabled.

		// Generally speaking, functions of BACCABLE are increasing, therefore to give more flexibility I set more
		// function enabling variable like SHIFT_INDICATOR_ENABLED, in order to allow you to select them.
		// It's up to you to select desired combination. If the combination increases too much the main loop duration,
		// the device will not work properly and the red led will start to continuously blink (you can find in the main
		// loop, the loop duration check)

		//note: with the following we avoid some combinations of defines, but not all combinations are considered. some of them to avoid are up to you.
		#ifdef ACT_AS_CANABLE
			#ifdef LED_STRIP_CONTROLLER_ENABLED
				#error "invalid combination of defines. ACT_AS_CANABLE and LED_STRIP_CONTROLLER_ENABLED both uses USB pins"
			#endif
			#ifdef IMMOBILIZER_ENABLED
				#error "invalid combination of defines. Disable ACT_AS_CANABLE to use IMMOBILIZER_ENABLED"
			#endif
		#endif

		#ifdef SHOW_PARAMS_ON_DASHBOARD
			#if (defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ESC_TC_CUSTOMIZATOR_ENABLED))  //if required, let's automatically open the can bus
				#error "invalid combination of defines. Disable SHOW_PARAMS_ON_DASHBOARD if you want to use other functionalities"
			#endif
		#endif

	#if defined(LED_STRIP_CONTROLLER_ENABLED)
		#include "vuMeter.h" //this is used to control led strip through usb pin
	#endif

	#include "stm32f0xx_hal.h"
	#include "onboardLed.h"
	#include "can.h"
	#include "slcan.h"

	void SystemClock_Config(void);
	void system_irq_disable(void);
	void system_irq_enable(void);
	void system_hex32(char *out, uint32_t val);
	void Error_Handler(void);
	float scaleVolume(uint8_t vol);
	uint8_t scaleColorSet(uint8_t col);
	#ifdef __cplusplus
		}
	#endif

#endif /* __MAIN_H */
