#ifndef __MAIN_H
#define __MAIN_H

	#include "globalVariables.h"
	#include "functions_Common.h"

<<<<<<< Updated upstream
		#include "globalVariables.h"




	#if defined(C2baccable)
		void dynoToggle();
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

	#if defined(C1baccable)
		void sendDashboardPageToSlaveBaccable(float param);
		void sendMainDashboardPageToSlaveBaccable();
		void sendSetupDashboardPageToSlaveBaccable();
	#endif

	#ifdef __cplusplus
		}
	#endif
=======
	#include "processingExtendedMessage.h"
	#include "processingStandardMessage.h"

	#if defined(C1baccable)
		#include "functions_C1baccable.h"
	#endif

	#if defined(BHbaccable)
		#include "functions_BHbaccable.h"
	#endif

	// the #defines were moved to globalVariables.h

>>>>>>> Stashed changes
#endif /* __MAIN_H */
