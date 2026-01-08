/*
 * C1baccableFunctions.h
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#ifndef INC_FUNCTIONS_C1BACCABLE_H_
	#define INC_FUNCTIONS_C1BACCABLE_H_
	#include "globalVariables.h"
	#include "functions_Common.h"

	void C1baccableInitCheck(void);
	void C1baccablePeriodicCheck(void);
	void sendMainDashboardPageToSlaveBaccable(void);
	void sendSetupDashboardPageToSlaveBaccable(void);
	void sendParamsSetupDashboardPageToSlaveBaccable(void);
	void sendDashboardPageToSlaveBaccable(void);
	float getNativeParam(uint8_t paramId);
	void buildLineWithFormat(const char* template, float values[2], const uint8_t paramId[2], char* result);
	uint8_t removePatterns(char *str);
	uint32_t getFreeRAM(void);
	void clearDashboardBaccableMenu(void);
	float scaleVolume(uint8_t vol);
	uint8_t scaleColorSet(uint8_t col);
	uint8_t saveOnflash(void);
	uint8_t resetStatisticsOnFlash();
	uint8_t saveStatisticsOnFlash(void);
	uint8_t saveShownParamsOnflash(void);
	void compress_to_uint16(const uint8_t *input, size_t input_len, uint16_t *output);
	void decompress_from_uint16(const uint16_t *input, size_t input_len, uint8_t *output, size_t output_len);
	void readShownParamsFromFlash(void);
	uint16_t readFromFlash(uint8_t paramId);
	float readStatisticsFromFlash(uint8_t paramId);
	uint8_t getParamIndexFromReqId(uint32_t searchedReqId);
	uint8_t getNextVisibleParam(uint8_t curIndex);
	uint8_t getPreviousVisibleParam(uint8_t curIndex);
#endif /* INC_FUNCTIONS_C1BACCABLE_H_ */
