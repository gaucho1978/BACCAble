/*
 * functions_BHbaccable.h
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#ifndef INC_FUNCTIONS_BHBACCABLE_H_
	#define INC_FUNCTIONS_BHBACCABLE_H_

	#include "globalVariables.h"

	void BHbaccableInitCheck();
	void BHperiodicCheck();
	uint8_t saveOnFlashBH();
	uint16_t readFromFlashBH(uint8_t paramId);

#endif /* INC_FUNCTIONS_BHBACCABLE_H_ */
