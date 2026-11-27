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
		uint8_t		replyFlags;		//bit0 = raw value is signed (8 or 16 bit). Sits in a padding byte, the struct stays 40 bytes
		int32_t		replyValOffset;
		float		replyScale;
		int32_t		replyScaleOffset;
		uint8_t		replyMeasurementUnit[7];
		uint8_t		replyDecimalDigits;
	} uds_param_single_element;
	_Static_assert(sizeof(uds_param_single_element)==40, "uds_param_single_element grew"); //table layout check
	#define UDS_REPLY_SIGNED 0x01 //signed UDS decoding
	#define SHOWN_PARAMS_MARK 0x6A31		//last word of the page visibility area: written by this firmware
	#define UPSTREAM_PAGES_GASOLINE 47		//pages whose visibility bit older firmware stored
	#define OBD_PAGES_FIRST 61				//gasoline pages that send OBD functional requests: hidden until enabled
	#define OBD_PAGES_LAST 64
	void shownParamsUpgrade(uint8_t *shown, uint16_t mark, uint8_t isDiesel);
	#define UDS_REPLY_OBD 0x02 //OBD mode 01 request (02 01 PID), reply is 41 PID data: data start at byte 3
	#define UDS_PAGES_MAX 72 //pages per fuel type, was 60. One visibility bit per page: must stay <= 240
	#define UDS_SINGLE_PARAMS_MAX 130 //parameter records, was 100. Index is uint8_t: must stay <= 255
	#define WARMUP_OIL_RAW 120 //oil 80 C as raw byte of 0x4B2 (raw-40)
	#define WARMUP_REARM_MS 60000 //engine off longer than this: next start is a new warm-up
	void warmupTimerUpdate(uint32_t nowMs, uint32_t rpm, uint8_t oilRaw); //warm-up timer
	float warmupTimerMinutes(void); //warm-up timer
	uint16_t screenRefreshPeriodMs(uint8_t mode); //screen rate
	uint8_t udsDecodeReply(const uds_param_single_element *p, const uint8_t *rx, uint8_t dlc, float *out); //signed UDS decoding

	extern float dashboardParamCouple[2];
	extern float dashboardParamMaxHold[2];
	extern uint8_t shownParamsArray[240];
	_Static_assert(UDS_PAGES_MAX<=240 && UDS_SINGLE_PARAMS_MAX<=255, "table sizes"); //table size check
	extern uint8_t params_setup_dashboardPageIndex;
	extern uint8_t total_pages_in_params_setup_dashboard_menu;
	extern uint8_t total_pages_in_dashboard_menu_gasoline;
	extern uint8_t total_pages_in_dashboard_menu_diesel;
	extern const uds_params_couple_element uds_params_array[2][UDS_PAGES_MAX];
	extern const uds_param_single_element single_uds_params_array[UDS_SINGLE_PARAMS_MAX];
	extern uint8_t currentParamElementSelection;

	extern const char* dpfRegenEnumStrings[];
	extern const char* setbeltEnumStrings[];
	extern const uint8_t gearArray[11];
	extern const char* speedStatisticEnumStrings[];


#endif /* INC_UDS_PARAMETERS_H_ */
