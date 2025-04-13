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
			//#define C1baccable //uncomment this to compile firmware for baccable on C1 can bus
			//#define C2baccable //uncomment this to compile firmware for baccable on C2 can bus
			#define BHbaccable //uncomment this to compile firmware for baccable on BH can bus
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
			//#define UCAN_BOARD_LED_INVERSION //uncommented on ucan fysect board (and on new baccable board). the led onboard are physically connected differently (status is inverted)

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
			#define UCAN_BOARD_LED_INVERSION //uncommented on ucan fysect board (and on new baccable board). the led onboard are physically connected differently (status is inverted)

			#define ESC_TC_CUSTOMIZATOR_ENABLED // enable/disable ESC and Traction control (also controlled by C1 baccable) (by pressing LANE button (left stak) for 2 seconds it inverts current enabling status of ESC and TC features). it works only in race mode
			#define DYNO_MODE //disables all the controls (roll bench mode of MES) (also controlled by C1 baccable).
			#define FRONT_BRAKE_FORCER //disables front brakes (controlled by C1baccable).
			#define CLEAR_FAULTS_ENABLED //if enabled the C1baccable menu will allow to reset faults thru C2baccable too
		#endif

		#if defined(BHbaccable) //this works only on BH can bus (obd port pin 3 and pin 11)
			#pragma message("Building BH BACCAble")
			#define UCAN_BOARD_LED_INVERSION //uncommented on ucan fysect board (and on new baccable board). the led onboard are physically connected differently (status is inverted)

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

			void sendDashboardPageToSlaveBaccable(float param);
			void sendMainDashboardPageToSlaveBaccable();
			void sendSetupDashboardPageToSlaveBaccable();
		#endif



	//this is used to invert bytes order in a 32 bit integer
	#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) | \
						   (((uint32_t)(x) >> 8)  & 0x0000FF00) | \
						   (((uint32_t)(x) << 8)  & 0x00FF0000) | \
						   (((uint32_t)(x) << 24) & 0xFF000000)

	#define LAST_PAGE_ADDRESS (FLASH_BANK1_END - FLASH_PAGE_SIZE +1) // 0x0801F800 //valid only for stm32F072 i suppose
			//la flash inizia a 0x08000000  e finisce a 0x0801FFFF, -0x800 +1 di una pagina fa 0x0801F800

	#if defined(C2baccable)
		void dynoToggle(void);
	#endif

	void SystemClock_Config(void);
	void system_irq_disable(void);
	void system_irq_enable(void);
	void system_hex32(char *out, uint32_t val);
	void Error_Handler(void);

	float scaleVolume(uint8_t vol);
	void floatToStr(char* str, float num, uint8_t precision,uint8_t maxLen);
	uint8_t scaleColorSet(uint8_t col);
	uint8_t saveOnflash();
	uint16_t readFromFlash(uint8_t paramId);

	void clearDashboardBaccableMenu();

	uint8_t calculateCRC(uint8_t* data, uint8_t arraySize);

	#ifdef __cplusplus
		}
	#endif





#endif /* __MAIN_H */
