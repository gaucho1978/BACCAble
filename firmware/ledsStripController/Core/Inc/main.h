#ifndef __MAIN_H
#define __MAIN_H

	#ifdef __cplusplus
		extern "C" {
	#endif

	#include "stm32f0xx_hal.h"
	#include "onboardLed.h"
	#include "can.h"
	#include "slcan.h"




	//#define ACT_AS_CANABLE //uncomment this to use the canable connected to the pc, as a usb-can adapter

	#define DISABLE_START_STOP //uncomment this to automatically disable start&stop car functionality
	#define IMMOBILIZER_ENABLED //comment this if you don't want immobilizer functionality. This functionality waits for a thief to connect to RFHUB and if the connection message is found, it resets the rfhub and it starts the Panic alarm.

	#if defined(ACT_AS_CANABLE)
	#else
		#include "vuMeter.h"
	#endif /* ACT_AS_CANABLE */

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
