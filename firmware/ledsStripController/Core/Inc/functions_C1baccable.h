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

	void C1baccableInitCheck();
	void C1baccablePeriodicCheck();
	void sendMainDashboardPageToSlaveBaccable();
	void sendSetupDashboardPageToSlaveBaccable();
	void sendDashboardPageToSlaveBaccable(float param);
	void clearDashboardBaccableMenu();
	float scaleVolume(uint8_t vol);
	uint8_t scaleColorSet(uint8_t col);
	uint8_t saveOnflash();
	uint16_t readFromFlash(uint8_t paramId);


#endif /* INC_FUNCTIONS_C1BACCABLE_H_ */
