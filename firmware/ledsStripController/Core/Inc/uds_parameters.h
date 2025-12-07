/*
 * uds_parameters.h
 *
 *  Created on: Nov 22, 2025
 *      Author: emanu
 */

#ifndef INC_UDS_PARAMETERS_H_
	#define INC_UDS_PARAMETERS_H_
	#include "compile_time_defines.h"
	#include "stm32f0xx_hal.h"

	//this is used to invert bytes order in a 32 bit integer
	#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) | \
						   (((uint32_t)(x) >> 8)  & 0x0000FF00) | \
						   (((uint32_t)(x) << 8)  & 0x00FF0000) | \
						   (((uint32_t)(x) << 24) & 0xFF000000)
	typedef struct{
		char		name[DASHBOARD_MESSAGE_MAX_LENGTH +5];
		uint8_t 	udsParamId[2];
	} uds_params_couple_element;

	typedef struct{
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
	} uds_param_single_element;

	extern float dashboardParamCouple[2];
	extern uint8_t shownParamsArray[240];
	extern uint8_t params_setup_dashboardPageIndex;
	extern uint8_t total_pages_in_params_setup_dashboard_menu;
	extern uint8_t total_pages_in_dashboard_menu_gasoline;
	extern uint8_t total_pages_in_dashboard_menu_diesel;
	extern const uds_params_couple_element uds_params_array[2][60];
	extern const uds_param_single_element single_uds_params_array[100];
	extern uint8_t currentParamElementSelection;

	extern const char* dpfRegenEnumStrings[];
	extern const char* setbeltEnumStrings[];
	extern const uint8_t gearArray[11];
	extern const char* speedStatisticEnumStrings[];


#endif /* INC_UDS_PARAMETERS_H_ */
