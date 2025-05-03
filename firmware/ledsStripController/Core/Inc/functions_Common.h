/*
 * functions_Common.h
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#ifndef INC_FUNCTIONS_COMMON_H_
#define INC_FUNCTIONS_COMMON_H_
	#include "globalVariables.h"

	void floatToStr(char* str, float num, uint8_t precision, uint8_t maxLen);
	uint8_t calculateCRC(uint8_t* data, uint8_t arraySize);

	void SystemClock_Config(void);
	void system_irq_disable(void);
	void system_irq_enable(void);
	void system_hex32(char *out, uint32_t val);
	void Error_Handler(void);

#endif /* INC_FUNCTIONS_COMMON_H_ */
