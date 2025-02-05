#ifndef __MAIN_H
#define __MAIN_H

	#ifdef __cplusplus
		extern "C" {
	#endif




		#include "stm32f0xx_hal.h"
		#include "onboardLed.h"
		#include "can.h"
		#include "slcan.h"
		#include "math.h"

		//#define ACT_AS_CANABLE //uncomment this to use the canable connected to the pc, as a usb-can adapter, for sniffing purposes
		#define SMART_DISABLE_START_STOP //comment this if you don't want to use start&stop disabling with can message
		//#define DISABLE_START_STOP //uncomment this if you want to use start&stop disabling with external resistor simulating button press. this is left just in case the smart approach on some cars dont't work.

		#define IMMOBILIZER_ENABLED //this works only on C1 can bus (OBD port pins 6 and 14) //comment this if you don't want immobilizer functionality. This functionality waits for a thief to connect to RFHUB and if the connection message is found, it resets the rfhub and it starts the Panic alarm.
		//#define LED_STRIP_CONTROLLER_ENABLED //this was tested only on C1 can bus (OBD port pins 6 and 14) //comment this if you don't want led strip controller functionality //NOTE: it can not be used with USE_AS_CANABLE cause both uses usb port
		//#define SHIFT_INDICATOR_ENABLED //this was tested only on C1 can bus (OBD port pins 6 and 14) //comment this line if you don't want to show shift indicator when rpm motor goes over the configurable threshold SHIFT_THRESHOLD (in race mode)
		//#define SHIFT_THRESHOLD 4500 //this is the configurable shift threshold. 2 more thresholds are automatically defined: 500rpm and 1000 rpm higher than SHIFT_THRESHOLD value
		//#define SHOW_PARAMS_ON_DASHBOARD //this works only on BH can bus (obd port pin 3 and pin 11) //--// uncomment this if you connected another baccable to usb port and want this baccable to receive parameters from master baccable. Received parameter will be displaied on the dashboard.
		#define SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE //this works only on C1 can bus (OBD port pins 6 and 14) //uncomment this if you connected another baccable to usb port and want this baccable to send parameters to slave baccable (the slave will display parameter on the dashboard). if defined, the cruise control buttons + and - will change the shown parameter

		//#define ROUTE_MSG 	//this define performs the following:
							// upon UDS request with message id 0x18DABAF1, with uds command 082201xxyyyyyyyy,
							// it will route specified native message to the sender.
							// it is done just one time to avoid bus flood
							// xx is 0x00 for stdId, 0x01 for Ext Id. yyyyyyyy is the requested msg id right aligned.

		//#define ESC_TC_CUSTOMIZATOR_ENABLED //this works only on C2 can bus (obd port pin 12 and 13) //--// uncomment this line if you want to be able to enable/disable ESC and Traction control (pressing LANE button (left stak) for 2 seconds it inverts current status of ESC and TC features, so if they are enabled, they will be disabled and viceversa)
		//#define DYNO_MODE

		//#define UCAN_BOARD_LED_INVERSION //on ucan fysect board the led onboard are physically connected differently (status is inverted)

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
		#if (defined(ACT_AS_CANABLE) &&									(defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ESC_TC_CUSTOMIZATOR_ENABLED) || defined(DYNO_MODE) || defined(SHOW_PARAMS_ON_DASHBOARD) ))
			#error "invalid combination of defines. ACT_AS_CANABLE can not be enabled because other fuctions are enabled"
		#endif

		#if (defined(SHOW_PARAMS_ON_DASHBOARD) &&						(defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ESC_TC_CUSTOMIZATOR_ENABLED) || defined(DYNO_MODE) ||defined(ACT_AS_CANABLE) ))  //if required, let's automatically open the can bus
			#error "invalid combination of defines. Disable SHOW_PARAMS_ON_DASHBOARD (because it works on BH can bus) if you want to use other functionalities (that works on C1/C2 can bus)"
		#endif

		#if ((defined(ESC_TC_CUSTOMIZATOR_ENABLED) || defined(DYNO_MODE)) &&	(defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ACT_AS_CANABLE) || defined(SHOW_PARAMS_ON_DASHBOARD) ))
			#error "invalid combination of defines. If you want ESC_TC_CUSTOMIZATOR_ENABLED or DYNO, disable the other functions"
		#endif
		#if (defined(SMART_DISABLE_START_STOP) && defined(DISABLE_START_STOP))
			#error "invalid combination of defines. Choose SMART_DISABLE_START_STOP or DISABLE_START_STOP."
		#endif




		#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
			typedef struct{
				uint8_t		name[15];
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

			void sendDashboardPageToSlaveBaccable(float param);
		#endif



	//this is used to invert bytes order in a 32 bit integer
	#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) | \
						   (((uint32_t)(x) >> 8)  & 0x0000FF00) | \
						   (((uint32_t)(x) << 8)  & 0x00FF0000) | \
						   (((uint32_t)(x) << 24) & 0xFF000000)

	#define LAST_PAGE_ADDRESS (FLASH_BANK1_END - FLASH_PAGE_SIZE +1) // 0x0801F800 //valid only for stm32F072 i suppose
			//la flash inizia a 0x08000000  e finisce a 0x0801FFFF, -0x800 +1 di una pagina fa 0x0801F800

	void SystemClock_Config(void);
	void system_irq_disable(void);
	void system_irq_enable(void);
	void system_hex32(char *out, uint32_t val);
	void Error_Handler(void);

	float scaleVolume(uint8_t vol);
	void floatToStr(char* str, float num, uint8_t precision,uint8_t maxLen);
	uint8_t scaleColorSet(uint8_t col);
	uint8_t saveOnflash(uint16_t param1);
	uint16_t readFromFlash(uint8_t paramId);

	void clearDashboardBaccableMenu();

	#ifdef __cplusplus
		}
	#endif





#endif /* __MAIN_H */
