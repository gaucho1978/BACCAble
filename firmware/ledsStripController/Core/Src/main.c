// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"

#if defined(ACT_AS_CANABLE) // || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	#include "usb_device.h"
	#include "usbd_cdc_if.h"
	#include "string.h"
#endif

#if defined(SHOW_PARAMS_ON_DASHBOARD) || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	#include "uart.h"
#endif

#if defined(LED_STRIP_CONTROLLER_ENABLED)
	#include "vuMeter.h" //this is used to control led strip through usb pin
#endif

const char *FW_VERSION="BACCABLE V.2.3";  //this is used to store FW version, also shown on usb when used as slcan

#if defined(UCAN_BOARD_LED_INVERSION)
	const uint8_t led_light_on_bit=1;
#else
	const uint8_t led_light_on_bit=0;
#endif


#if defined(LED_STRIP_CONTROLLER_ENABLED)
	float scaledVolume;
	uint8_t scaledColorSet;
	uint32_t timeSinceLastReceivedAcceleratorMessage=0;
	uint8_t ledsStripIsOn=0; //indicates if leds strip is on
#endif

#if(defined(IMMOBILIZER_ENABLED))
	uint8_t immobilizerEnabled=1; //parameter stored in ram, so that we can change it dinamically
	uint8_t panicAlarmActivated=0; //indicates if the panic alarm was activated during last... 10 minutes (ToBeVerified)
	//the following 2 arrays declares: RFHUB reset (first message) and panic alarm messages definition (the others)
	CAN_TxHeaderTypeDef panicAlarmStartMsgHeader[5]={
			{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAC7F1, .DLC=3},
			{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x1E340041, .DLC=4},
			{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
			{.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8},
	};

	uint8_t panicAlarmStartMsgData[5][8]={
			{0x02,0x11,0x01,0x00,},
			{0x88,0x20,0x15,0x00,},
			{0x42,0x02,0xE2,0x00,0x00,0x00,0x01,0x56},
			{0x00,0x00,0xE2,0x00,0x00,0x00,0x00,0x00},
	};

	CAN_TxHeaderTypeDef dashboardBlinkMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x545, .DLC=8};
	uint8_t dashboardBlinkMsgData[8]= {0x88, 0x20, 0xC3, 0x24, 0x00, 0x14, 0x30, 0x00};
	uint32_t last_sent_dashboard_blink_msg_time=0;
	uint8_t floodTheBus=0;
	uint32_t floodTheBusStartTime=0;
	uint32_t floodTheBusLastTimeSent=0;
	uint8_t executeDashboardBlinks=0; //executes the number of defined blinks, one each second.
#endif

#if (defined(SMART_DISABLE_START_STOP) || defined(DISABLE_START_STOP))
	CAN_TxHeaderTypeDef disableStartAndStopMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x4B1, .DLC=8};
	uint8_t disableStartAndStopMsgData[8]= {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; //byte 5 shall be set to 0x08
	uint8_t startAndStopEnabled=1; //this is the status of my internal logic. If=0 the function goes to sleep up to next reboot
	uint8_t startAndstopCarStatus=1; //this is the status of Start&stop received by the car. 1=enabled in car (this is the default status in giulias).
	uint32_t lastTimeStartAndstopDisablerButtonPressed=0;
#endif

#if (defined(SHIFT_INDICATOR_ENABLED))
	CAN_TxHeaderTypeDef shift_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2ED, .DLC=8}; //used when SHIFT_INDICATOR_ENABLED is defined
	uint8_t shift_msg_data[8]; //used when SHIFT_INDICATOR_ENABLED is defined
#endif

#if(defined(ESC_TC_CUSTOMIZATOR_ENABLED))
	uint8_t currentDNAmode; //used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
	uint8_t DNA_msg_data[8];//used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
	CAN_TxHeaderTypeDef DNA_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8}; //used when ESC_TC_CUSTOMIZATOR_ENABLED is defined
	uint8_t ESCandTCinversion=0; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)
	uint32_t LANEbuttonPressLastTimeSeen=0; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
	uint8_t LANEbuttonPressCount=0; //stores number of times this message field was received
#endif

#if(defined(DYNO_MODE))
	uint8_t DynoModeEnabled=0;
	uint8_t DynoStateMachine=0xff; //State machine for dyno messages sequence. frm 00 to 03 = dyno message sequence is beeing transmitted. FF= inactive
	uint16_t testerMsgSent=0;
	uint8_t DYNO_msg_data[5][6]={{0x02,0x10,0x03,},{0x03,0x22,0x30,0x02,},{0x05,0x2E,0x30,0x02,0x00,0x01},{0x05,0x2E,0x30,0x02,0xFF,0x01}}; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno
	CAN_TxHeaderTypeDef DYNO_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=6};

	uint32_t DynoStateMachineLastUpdateTime=0; //stores time (in milliseconds from power on) when Park Assist button press was read last time
	uint8_t ParkAssistButtonPressCount=0; //stores number of times this message field was received


#endif

#if (defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE))
uint8_t total_pages_in_dashboard_menu=34;
const	uds_param_element uds_params_array[60]={
								{.name={}, 															.reqId=0,       .reqLen=0,	.reqData=0,                 			.replyId=0,				.replyLen=0,    .replyOffset=0,	.replyValOffset=0,		.replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,	.replyMeasurementUnit={}								},
								{.name={'P','O','W','E','R',':',' ',},								.reqId=0x11,	.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),		.replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-500,	.replyScale=0.000142378,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'C','V',}						}, //devo ricordare di moltiplicare il risultato per RPM
								{.name={'T','O','R','Q','U','E',':',' ',},							.reqId=0x12,	.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),		.replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=-500,	.replyDecimalDigits=0,	.replyMeasurementUnit={'N','m',}						},
								{.name={'D','P','F',':',' ',},										.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218E4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'D','P','F',':',' ',},										.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218DE),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'D','P','F',' ','R','E','G','E','N',':',' ', },				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322380B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.001525902,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'L','A','S','T',' ','R','E','G','E','N','.',':',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223807),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
								{.name={'T','O','T',' ','R','E','G','E','N',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218A4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
								{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223809),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
								{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322380A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01666666666,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','i','n',}					},
								{.name={'B','A','T','T','.',':',},									.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221955),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0005,			.replyScaleOffset=0,	.replyDecimalDigits=3,	.replyMeasurementUnit={'V',}							},
								{.name={'B','A','T','T','.',':',},									.reqId=0x13, 		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'B','A','T','T','.',':',},									.reqId=0x14,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-250,	.replyDecimalDigits=2,	.replyMeasurementUnit={'A',}							},
								{.name={'O','I','L',' ','D','E','G','R','.',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223813),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'O','I','L',':',' ',},										.reqId=0x15,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x4B2,			.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=-40,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'O','I','L',':',' ',},										.reqId=0x10,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000004B2,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
								{.name={'A','I','R',' ','I','N',':',' ',},							.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221935),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'C','U','R','.',' ','G','E','A','R',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192D),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
//may be not received.			{.name={'G','E','A','R',':',' ',},									.reqId=0x16,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x000001F7,	.replyLen=1,	.replyOffset=0,	.replyValOffset=0,		.replyScale=3,				.replyScaleOffset=-40,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'W','A','T','E','R',':',' ',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221003),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
//	tyres commented since		{.name={'F','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B1),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//	they connect to rfhub		{.name={'F','-','R',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B2),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//	and immobilizer starts		{.name={'R','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B3),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//								{.name={'R','-','R',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B4),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
//may be not received			{.name={'E','G','R',' ','C','M','D','1',':',},						.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322012C),	.replyId=0x18DBF133,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.3921568627,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'E','G','R',' ','C','M','D',':',},							.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32767,	.replyScale=0.00305185095,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'E','G','R',':',' ',},										.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189A),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.1953125,		.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
//sometimes wrong value			{.name={'E','G','R',' ','M','E','A','S','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189C),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32767,	.replyScale=0.00305185095,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
//wrong value 43520g			{.name={'P','A','R','T','I','C','U','L','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218AA),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'g',}							},
//may be not received			{.name={'T','U','R','B','O','1',':',},								.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03220175),	.replyId=0x18DBF133,	.replyLen=2,	.replyOffset=5,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
//may be not received			{.name={'T','U','R','B','O','4',':',},								.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322010B),	.replyId=0x18DBF133,	.replyLen=1,	.replyOffset=0,	.replyValOffset=-100,	.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
//stuck to 3,18V				{.name={'T','U','R','B','O','5',':',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221936),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							},
								{.name={'T','U','R','B','O',' ','R','E','Q',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.000030517578,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
								{.name={'T','U','R','B','O',' ','R','E','Q',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322189F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.00152590219,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'T','U','R','B','O',':',},									.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221935),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'T','U','R','B','O',':',},									.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322195A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=-32768, .replyScale=0.001,			.replyScaleOffset=-1,	.replyDecimalDigits=2,	.replyMeasurementUnit={'b','a','r',}					},
								{.name={'T','U','R','B','O',':',' ',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218A0),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.00152590219,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
								{.name={'B','O','O','S','T',' ','R','E','Q','.',':',' ',},			.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221959),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-32768,	.replyScale=0.001,			.replyScaleOffset=-1,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
								{.name={'B','O','O','S','T',':',' ',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322195B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							},
								{.name={'R','A','I','L',':',' ',},									.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221904),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','P','a',}					},
//redundant(same as rail maybe)	{.name={'D','I','E','S','E','L',':',' ',},							.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221947),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
								{.name={'D','I','E','S','E','L',':',' ',},							.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221900),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
								{.name={'O','D','O','M','.','L','A','S','T',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222002),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
								{.name={'A','I','R',' ','C','O','N','D','.',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
//may be not received			{.name={'F','U','E','L',':',' ',},									.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03220123),	.replyId=0x18DBF133,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=10,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','P','a',}					},
								{.name={'F','U','E','L',' ','C','O','N','S','.',':',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0000394789,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'L','/','h',}					},
								{.name={'D','E','B','I','M','E','T','E','R',':',},					.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322193F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},

		}; // initializes all the uds parameters request to send and reply to receive - it is initialized with data from the defines in main.h, in order to avoid to touch this declaration - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality

	CAN_TxHeaderTypeDef uds_parameter_request_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA10F1, .DLC=3}; //used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
	uint8_t baccableDashboardMenuVisible=0;
	uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
	uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
	uint16_t torque; //torque
	uint8_t batteryStateOfCharge=0; //battery charge %
	uint16_t batteryCurrent; //battery current (to be converted in Amps)
	uint8_t transmissionTemperature;
	uint8_t uds_parameter_request_msg_data[8];//used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
	uint8_t dashboardPageIndex=0; //to send message index - it changes when you press cruise control buttons - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality.
	uint32_t last_sent_uds_parameter_request_Time=0; //stores last time we send a uds parameter request - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality
#endif

#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED)
	uint8_t cruiseControlDisabled=1;
	uint8_t wheelPressedButtonID=0x10; //0x10= released, 0x20=strong speed decrease, 0x18=speed decrease, 0x00=strong speed increase, 0x08=speed increase, 0x90=RES, CC on/off=0x12
	uint8_t  lastPressedWheelButton=0xff; //default value, means no button pressed on the wheel
	uint32_t lastPressedWheelButtonTime=0;//stores the last time a wheel button was pressed, in msec from boot
	uint32_t lastPressedWheelButtonDuration=0x00; //default value
	uint32_t lastPressedSpeedUpWheelButtonDuration=0x00; //default value
#endif

#if (defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(SHOW_PARAMS_ON_DASHBOARD))
	uint8_t dashboardPageStringArray[18]; //used if SHOW_PARAMS_ON_DASHBOARD or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is declared - it contains string to print on dashboard
	uint8_t uartTxMsg[UART_BUFFER_SIZE]; //used if SHOW_PARAMS_ON_DASHBOARD or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is declared - it contains string to send over uart
#endif

#if (defined(SHOW_PARAMS_ON_DASHBOARD))
	uint32_t lastSentTelematic_display_info_msg_Time=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	uint8_t telematic_display_info_field_totalFrameNumber=5; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_frameNumber=0; //current frame //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_infoCode=0x0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t paramsStringCharIndex=0; // next char to send index - Used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8}; //used when SHOW_PARAMS_ON_DASHBOARD is defined
	uint8_t telematic_display_info_msg_data[8]; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t requestToSendOneFrame=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard
#endif

#if defined(ROUTE_MSG)
	uint8_t routeStdIdMsg=0xff;
	uint8_t routeOffset=0;
	uint32_t routeMsgId=0;

	CAN_TxHeaderTypeDef routeMsgHeader={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAF1BA, .DLC=8};
	uint8_t routeMsgData[8]= {0x07, 0x62, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

#endif

	uint32_t debugTimer0;

	UART_HandleTypeDef huart2; // this is the serial line between baccables -- used with SHOW_PARAMS_ON_DASHBOARD and SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionalities

	uint32_t currentRpmSpeed=0;//used when SHIFT_INDICATOR_ENABLED or IMMOBILIZER_ENABLED or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE or DISABLE_START_STOP is defined
	uint8_t currentGear=0; // used when IMMOBILIZER_ENABLED or LED_STRIP_CONTROLLER_ENABLED is defined
	// Storage for status and received message buffer
	CAN_RxHeaderTypeDef rx_msg_header;  //msg header
	uint8_t rx_msg_data[8] = {0};  //msg data
	uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

	//the following array stores buttons pressed password sequence (future growth now commented)
	//these are possible values 0x90=RES,
	//							0x11=Adaptive Cruise control on/off
	//							0x12=Cruise control on/off,
	//                          0x08=Cruise control speed gently up,
	//                          0x00=Cruise control speed strong up,
	//                          0x18=Cruise control speed gently down,
	//                          0x20=Cruise control speed strong down
	// WARNING: when you press cruise control strong up, before and after it, also cruise control gently up
	//          message is fired, therefore your sequence will be altered. A workaround
	//          is to use both messages in the sequence, or to use just the gently up/down message.

	//uint32_t buttonPressedTimeArray[20] = {0};  //0=RES, 1=Cruise control on/off, 2=Cruise control speed gently up, 3=Cruise control speed strong up, 4=Cruise control speed gently down, 5=Cruise control speed strong down
	//uint8_t ButtonPressSequence1Index=0;
	//uint8_t ButtonPressSequence1Len=5;
	//uint8_t ButtonPressSequence1[5]={0x90,0x08,0x00,0x08,0x90}; //current password: RES - CC gently up  - CC strong up - CC gently up - RES

	//uint8_t ButtonPressSequence2Index=0;
	//uint8_t ButtonPressSequence2Len=8;
	//uint8_t ButtonPressSequence2[8]={0x90,0x08,0x00,0x08,0x18,0x20,0x18,0x12}; //current password: RES - CC gently up - CC strong up - CC gently up - CC gently down - CC strong down - CC gently down - CC on/off

int main(void){



	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	can_init(); //initialize can interface
	//onboardLed_red_on(); This line doesn't work cause hardware is still initiating

	//uint16_t tmpVar2=readFromFlash(1);
	#if defined(IMMOBILIZER_ENABLED)
		immobilizerEnabled = (uint8_t)readFromFlash(1);  //parameter1 stored in ram, so that we can get it. By default Immo is enabled
		if(immobilizerEnabled) executeDashboardBlinks=2; //shows the user that the immobilizer is active (or not)
	#endif


	#if defined(ACT_AS_CANABLE) //  || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(SHOW_PARAMS_ON_DASHBOARD))
		uart_init();
	#endif

	//#if defined(SHOW_PARAMS_ON_DASHBOARD)
	//	MX_USB_DEVICE_Init();
	//#endif

	#if (defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(ESC_TC_CUSTOMIZATOR_ENABLED) ||defined(DYNO_MODE) || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(DISABLE_START_STOP) || defined(ROUTE_MSG))  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(SHOW_PARAMS_ON_DASHBOARD))
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_125K);//set can speed to 125kpbs
		can_enable(); //enable can port

		//prepare msg to send:
		//total frame number is on byte 0 from bit 7 to 3
		telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0xF8) | ((telematic_display_info_field_totalFrameNumber<<3) & 0xF8);
		//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
		telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0x3F) | ((telematic_display_info_field_infoCode) & 0x3F);
		//I don't use UTF chars, but ascii, so bytes 2,4,6 can be set to zero
		telematic_display_info_msg_data[2]=0;
		telematic_display_info_msg_data[4]=0;
		telematic_display_info_msg_data[6]=0;

	#endif



	while (1){

		debugTimer0=HAL_GetTick();
		onboardLed_process();
		can_process();

		//onboardLed_red_blink(5);
		//onboardLed_red_on();

		#if defined(ACT_AS_CANABLE) // || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
			cdc_process(); //processa dati usb
			//just for test
			//char *data = "Hello World from USB CDC\n";
			//CDC_Transmit_FS((uint8_t*)data, strlen(data));
			//HAL_Delay (1000);

			//just for test, we can periodically send a packet to can bus
			//we can set the can bus in loopback mode, to receive back each sent message.
			// To set loopback mode, in function can_enable (can.c) we shall set can_handle.Init.Mode = CAN_MODE_LOOPBACK
			// This way we will receive whatever we send
			// Before to send can messages, speed shall be set and can port shall be enabled,
			//can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
			//can_enable(); //enable can port
			//Then we can prepare and send the following test packet
			//CAN_TxHeaderTypeDef testMsgHeader;
			//testMsgHeader.IDE= CAN_ID_STD;
			//testMsgHeader.RTR = CAN_RTR_DATA;
			//testMsgHeader.StdId=0x0412;
			//testMsgHeader.DLC=5;
			//uint8_t testMsgData[8] = {0};
			//testMsgData[0]=0x01;
			//testMsgData[1]=0x01;
			//testMsgData[2]=0x01;
			//testMsgData[3]=0x01;
			//testMsgData[4]=0xE6; //pedal position
			//can_tx(&testMsgHeader, testMsgData);
		#endif

		//#if defined(SHOW_PARAMS_ON_DASHBOARD)
		//	cdc_process(); //processa dati usb
		//#endif

		#if defined(LED_STRIP_CONTROLLER_ENABLED)
			//don't act as canable. One USB port pin is used to control leds.
			vuMeterInit(); //initialize leds strip controller - this is called many times to divide the operations on more loops
			if(ledsStripIsOn){ //if the strip is on,
				if(timeSinceLastReceivedAcceleratorMessage+10000<HAL_GetTick() ){ //if no can interesting message for 10 seconds,  shutdown the leds to save energy
					shutdownLedsStrip();
					ledsStripIsOn=0; //entriamo solo una volta
				}
			}

		#endif

		#if defined(IMMOBILIZER_ENABLED)
			if(executeDashboardBlinks>0){ //if we shall execute a blink to give a feedback to the user
				if((last_sent_dashboard_blink_msg_time+500)<HAL_GetTick()){ //enter here once each halfsecond
					last_sent_dashboard_blink_msg_time=HAL_GetTick();//return here after half second
					//change the message
					if(executeDashboardBlinks % 2==0){ //select one time reduced brightness and one time high brightness, so that in any condition the change is visible on the dashboard
						dashboardBlinkMsgData[4]=0x00; //max bright
					}else{
						dashboardBlinkMsgData[4]=0xF0; //reduced bright
					}
					executeDashboardBlinks = executeDashboardBlinks -1 ;//decrease blinks counter
					onboardLed_blue_on();
					//send the message
					can_tx(&dashboardBlinkMsgHeader, dashboardBlinkMsgData);
				}
			}


			if(immobilizerEnabled){
				//the following it is used only by IMMOBILIZER functionality
				if(floodTheBus){ //WHEN THIS IS ACTIVATED, THE THIEF WILL NOT BE ABLE TO CONNECT TO RFHUB, AND CAR WILL NOT SWITCH ON.
					if(floodTheBusLastTimeSent+10<HAL_GetTick()){
						can_tx(&panicAlarmStartMsgHeader[0], panicAlarmStartMsgData[0]); //sends the message on can bus that resets the connection to RFHUB
						floodTheBusLastTimeSent=HAL_GetTick();
					}

					if(!panicAlarmActivated){ //if panic alarm is not activated, we shall activate it after 1 second (1 second to avoid stop and start simultaneous)
						if(floodTheBusStartTime+1000<HAL_GetTick()){
							for (uint8_t i=0;i<15;i++){
								can_tx(&panicAlarmStartMsgHeader[1], panicAlarmStartMsgData[1]);
							}
							can_tx(&panicAlarmStartMsgHeader[2], panicAlarmStartMsgData[2]);
							panicAlarmActivated=1;
						}
					}
					if(floodTheBusStartTime+10000 < HAL_GetTick()){ //if the bus is flooded since 10 seconds, stop flooding it
						floodTheBus=0; //stop flooding
						//stop the panic alarm
						for (uint8_t i=0;i<15;i++){
							can_tx(&panicAlarmStartMsgHeader[1], panicAlarmStartMsgData[1]);
						}
						can_tx(&panicAlarmStartMsgHeader[2], panicAlarmStartMsgData[2]);
						panicAlarmActivated=0;
					}


				}
			}
		#endif
		#if defined(DISABLE_START_STOP) || defined(SMART_DISABLE_START_STOP)
			if(startAndStopEnabled){
				if(HAL_GetTick()>30000){ //first 30 seconds don't do anything to avoid to disturb other startup functions or immobilizer
					if(currentRpmSpeed>400){ //if motor is on

						if(startAndstopCarStatus==0){//if start & stop was found disabled in car, we don't need to do enything. Avoid to enter here; We enter here in example if board is switched when the car is running and S&S was still manually disabled by the pilot
							startAndStopEnabled=0;
						}else{
							if(lastTimeStartAndstopDisablerButtonPressed==0){ //first time we arrive here, go inside

								//if we are using smart function, try to disable it with can message
								#if defined(SMART_DISABLE_START_STOP)
									//set message data, byte 5, bits from 5 to 3 to binary 001.
									disableStartAndStopMsgData[5]=(disableStartAndStopMsgData[5] & ~0x38) | ((0x01<<3) & 0x38);
									can_tx(&disableStartAndStopMsgHeader, disableStartAndStopMsgData);
									onboardLed_blue_on();
									startAndStopEnabled=0; //done
								#endif
								#if defined(DISABLE_START_STOP)
									// We will now short gpio to ground in order to disable start&stop car functionality. This will simulate car start&stop button press
									HAL_GPIO_WritePin(START_STOP_DISABLER, 0); // I use swclk pin, pin37, PA14
									lastTimeStartAndstopDisablerButtonPressed=HAL_GetTick();
									onboardLed_blue_on();
								#endif
							}
							#if defined(DISABLE_START_STOP)
								if(lastTimeStartAndstopDisablerButtonPressed+500<HAL_GetTick()){ //if pressed since 500msec
									HAL_GPIO_WritePin(START_STOP_DISABLER, 1); //return to 1
									startAndStopEnabled=0;
									onboardLed_blue_on();
								}
							#endif
						}
					}
				}
			}
		#endif

		#if defined(SHOW_PARAMS_ON_DASHBOARD) //this is the baccable slave
			if(requestToSendOneFrame>0){ //if requested by a message received from master baccable
				//send one msg to write something on the dashboard each 50msec (one frame each 300msec)
				if (lastSentTelematic_display_info_msg_Time+50<HAL_GetTick()){
					lastSentTelematic_display_info_msg_Time=HAL_GetTick();
					//prepare msg to send:
					//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
					telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0x07) | ((telematic_display_info_field_frameNumber>>2) & 0x07);
					telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0xC0) | ((telematic_display_info_field_frameNumber<<6) & 0xC0);

					//UTF text 1 is on byte 2 and byte 3
					telematic_display_info_msg_data[3]=dashboardPageStringArray[paramsStringCharIndex];
					paramsStringCharIndex++; //prepare to send next char
					//UTF text 2 is on byte 4 (set to zero ) and byte 5
					telematic_display_info_msg_data[5]=dashboardPageStringArray[paramsStringCharIndex];
					paramsStringCharIndex++; //prepare to send next char
					//UTF text 3 is on byte 6 (set to zero) and byte 7
					telematic_display_info_msg_data[7]=dashboardPageStringArray[paramsStringCharIndex];
					paramsStringCharIndex++; //prepare to send next char
					//send it
					can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet

					telematic_display_info_field_frameNumber++; //prepare for next frame to send
					if( paramsStringCharIndex>=18) { //if we sent the entire string
						paramsStringCharIndex=0; //prepare to send first char of the string
						telematic_display_info_field_frameNumber=0; //prepare to send first frame
						requestToSendOneFrame -= 1;
						onboardLed_blue_on();
					}
				}
			}
		#endif

		#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
			//send a parameter request each xx msec if dashboard menu shall be visible
			//baccableDashboardMenuVisible=1; //force menu always on, just for debug
			if((last_sent_uds_parameter_request_Time+500<HAL_GetTick()) && baccableDashboardMenuVisible){
				last_sent_uds_parameter_request_Time=HAL_GetTick();
				if(uds_params_array[dashboardPageIndex].reqId>0xFF){ //if req id is greather than 0xFF it is a standard UDS request.
					//request current parameter to ECU
					uds_parameter_request_msg_header.ExtId=uds_params_array[dashboardPageIndex].reqId;


					memcpy(&uds_parameter_request_msg_data[0],&uds_params_array[dashboardPageIndex].reqData,uds_params_array[dashboardPageIndex].reqLen );
					uds_parameter_request_msg_header.DLC=uds_params_array[dashboardPageIndex].reqLen;
					//uds_parameter_request_msg_header.ExtId=0x18DA40F1;
					//uds_parameter_request_msg_header.DLC=4;
					//uds_parameter_request_msg_data[0]=0x03;
					//uds_parameter_request_msg_data[1]=0x22;
					//uds_parameter_request_msg_data[2]=0x10;
					//uds_parameter_request_msg_data[3]=0x05;
					//onboardLed_blue_on();
					can_tx(&uds_parameter_request_msg_header, uds_parameter_request_msg_data); //transmit the request
				}else{ //0xff reqId is a special value that we use to send particular values. now we use to send baccable FW version and oil pressure, for now
					sendDashboardPageToSlaveBaccable(-3400000000000000000);
				}
			}

		#endif

		#if defined(DYNO_MODE)
			if(DynoStateMachine!=0xff){ //if state machine in progress
				if(DynoStateMachineLastUpdateTime+4000< HAL_GetTick()){ //if older than 4 sec
					DynoStateMachine=0xff; //timeout. stop any sequence
				}
			}
		#endif

		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#endif

				if (rx_msg_header.RTR == CAN_RTR_DATA){
					switch(rx_msg_header.IDE){
						case CAN_ID_EXT:
							#if defined(IMMOBILIZER_ENABLED)
								if(immobilizerEnabled){
									//if it is a message of connection to RFHUB, reset the connection periodically, but start the panic alarm only once
									if(floodTheBus==0){ //if we are not flooding the bus
										if ((rx_msg_header.ExtId==0x18DAC7F1) ||( (rx_msg_header.ExtId==0x18DAF1C7) && (floodTheBusStartTime==0)) ){  // If msg from thief or (reply from rfhub && it is the first time that it occurs)
											//thief connected to RFHUB: we shall reset the RFHUB and start the alarm
											//start to flood the bus with the rfhub disconnect message
											floodTheBus=1;
											floodTheBusStartTime=HAL_GetTick();
											onboardLed_blue_on();
										}
									}
								} //end of immobilizer section
							#endif

							#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
								if ((rx_msg_header.ExtId==uds_params_array[dashboardPageIndex].replyId) && baccableDashboardMenuVisible){ //if we received udf message with current selected parameter, let's aquire it

									onboardLed_blue_on();
									if (rx_msg_header.DLC>=4+uds_params_array[dashboardPageIndex].replyOffset+uds_params_array[dashboardPageIndex].replyLen){

										uint8_t numberOfBytesToRead=uds_params_array[dashboardPageIndex].replyLen;
										// Limita il numero di byte a un massimo di 4 per evitare overflow
										if (numberOfBytesToRead > 4) {
											numberOfBytesToRead = 4;
										}
										uint32_t tmpVal=0; //take value of received parameter

										// Costruisce il valore a partire dai byte ricevuti
										for (size_t i = 0; i < numberOfBytesToRead; i++) {
											tmpVal |= ((uint32_t)rx_msg_data[4+uds_params_array[dashboardPageIndex].replyOffset+i]) << (8 * (numberOfBytesToRead - 1 - i));
										}

										tmpVal+=uds_params_array[dashboardPageIndex].replyValOffset;
										float tmpVal2 =tmpVal * uds_params_array[dashboardPageIndex].replyScale;
										tmpVal2 +=uds_params_array[dashboardPageIndex].replyScaleOffset;

										sendDashboardPageToSlaveBaccable(tmpVal2);//send parameter via usb

									}

								}
							#endif

							#if defined(ROUTE_MSG)

								if (rx_msg_header.ExtId==0x18DABAF1){ //if route request and dashboard menu not shown to avoid conflicts
									if (rx_msg_header.DLC>=7){
										routeStdIdMsg=!(rx_msg_data[2]>>4); //standard or extended msgID route request
										routeOffset=(rx_msg_data[2] & 0x0F); //offset from which start to copy
										routeMsgData[2]=rx_msg_data[2]; //copy in the response

										routeMsgId=	((uint32_t)rx_msg_data[3] << 24) |  // MSB
								                 	((uint32_t)rx_msg_data[4] << 16) |
													((uint32_t)rx_msg_data[5] << 8)  |
													((uint32_t)rx_msg_data[6]);       	 // LSB



										onboardLed_blue_on();
									}
								}

								#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
									if(baccableDashboardMenuVisible) routeStdIdMsg=0xff; //disables the route request, to avoid conflicts with show params functionality
								#endif


								if(routeStdIdMsg==0){ //if we have to do it (ext id route request)
									if(rx_msg_header.ExtId==routeMsgId){ //received msg to route
										routeStdIdMsg=0xFF; //set this to disable the request. only one message is routed to avoid bus flood
										if(routeOffset<rx_msg_header.DLC){ //send only if offset is correct
											uint8_t sizeToCopy=5; //
											if((rx_msg_header.DLC - routeOffset )<sizeToCopy) sizeToCopy=rx_msg_header.DLC - routeOffset;
											memcpy(&routeMsgData[3],&rx_msg_data[routeOffset],sizeToCopy);
											if(sizeToCopy<5) memset(&routeMsgData[3+sizeToCopy],0x00, 5-sizeToCopy);

											//send it
											can_tx(&routeMsgHeader, routeMsgData);
											onboardLed_blue_on();
										}

									}
								}
							#endif //end define ROUTE_MSG

							#if defined(DYNO_MODE)
								if (rx_msg_header.ExtId==0X18DAF128 && DynoStateMachine!=0xff ){ //if message from ABS ECU and Dyno state machine is in progress
									if (DynoStateMachine==0 && rx_msg_header.DLC>=3){ //we received a reply to diagnostic session request msg
										if(rx_msg_data[0]==0x06 && rx_msg_data[1]==0x50 && rx_msg_data[2]==0x03){ //if request was successful
											DynoStateMachine++; //send dyno sts msg
										}
									}
									if (DynoStateMachine==1 && rx_msg_header.DLC>=5){ //we received a reply to dyno status msg
										if(rx_msg_data[0]==0x05 && rx_msg_data[1]==0x62 && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
											DynoStateMachine++; //send dyno disable
											if(rx_msg_data[4]==0x00){ //if it is disabled, we shall enable it
												DynoModeEnabled=0;//refresh current status
												DynoStateMachine++;//send dyno enable
											}else{ //it is enabled, we shall disable it
												DynoModeEnabled=1;//refresh current status
											}
										}
									}
									if (DynoStateMachine==2 && rx_msg_header.DLC>=4){ //we received a reply to dyno disable msg
										if(rx_msg_data[0]==0x03 && rx_msg_data[1]==0x6E && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
											DynoModeEnabled=0;//success change complete
											DynoStateMachine=0xff; //disable state machine
											onboardLed_blue_on();
										}
									}
									if (DynoStateMachine==3 && rx_msg_header.DLC>=4){ //we received a reply to dyno enable msg
										if(rx_msg_data[0]==0x03 && rx_msg_data[1]==0x6E && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
											DynoModeEnabled=1;//success change complete
											DynoStateMachine=0xff; //disable state machine
											onboardLed_blue_on();
										}
									}

									if (DynoStateMachine!=0xff && rx_msg_header.DLC>=3){ //in any case
										if( rx_msg_data[1]==0x7F ){ //if request refused, abort all
											DynoStateMachine=0xff; //disable state machine
											onboardLed_blue_on();
										}
									}
									if(DynoStateMachine!=0xff){ //if we are running, send next message
										DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1;
										can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
										onboardLed_blue_on();
										DynoStateMachineLastUpdateTime=HAL_GetTick();//save last time it was updated
									}
								}
							#endif
							break;
						case CAN_ID_STD: //if standard ID
							#if defined(ROUTE_MSG)
								if(routeStdIdMsg==1){ //if we have to do it (std msg id route request


									if(rx_msg_header.StdId==routeMsgId){ //received msg to route
										routeStdIdMsg=0xFF; //set this to disable the request. only one message is routed to avoid bus flood
										if(routeOffset<rx_msg_header.DLC){ //send only if offset is correct
											uint8_t sizeToCopy=5; //
											if((rx_msg_header.DLC - routeOffset )<sizeToCopy) sizeToCopy=rx_msg_header.DLC - routeOffset;
											memcpy(&routeMsgData[3],&rx_msg_data[routeOffset],sizeToCopy);
											if(sizeToCopy<5) memset(&routeMsgData[3+sizeToCopy],0x00, 5-sizeToCopy);

											//send it
											can_tx(&routeMsgHeader, routeMsgData);
											onboardLed_blue_on();
										}
									}
								}
							#endif

							switch(rx_msg_header.StdId){ //messages in this switch is on C can bus, only when on different bus, the comments explicitly tells if it is on another can bus

								case 0x00000090:
									//on BH can bus, slow bus at 125kbps, this message contains:

									//total frame number is on byte 0 from bit 7 to 3
									//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
									//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
									//UTF text 1 is on byte 2 and byte 3
									//UTF text 2 is on byte 4 and byte 5
									//UTF text 3 is on byte 6 and byte 7
									break;
								case 0x000000FB:

									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(rx_msg_header.DLC>=4){
											torque= ((rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111));
										}
									#endif
									//torque is on byte 2 from bit 6 to 0 and byte 3 from bit 7 to 4.

									break;

								case 0x000000FC: //message to dashboard containing rpm speed and not only
									#if (defined(SHIFT_INDICATOR_ENABLED) || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(DISABLE_START_STOP) || defined(SMART_DISABLE_START_STOP) || defined(IMMOBILIZER_ENABLED) )
										if(rx_msg_header.DLC>=2){
											currentRpmSpeed=(rx_msg_data[0] *256 + (rx_msg_data[1] & ~0x3) )/4; //extract rpm speed
											//onboardLed_blue_on();
										}
									#endif

									#if (defined(DISABLE_START_STOP) || defined(SMART_DISABLE_START_STOP))
										if(currentRpmSpeed< 400 ) startAndStopEnabled=1; //if motor off, re-enable start&stop
									#endif
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(currentRpmSpeed<400) baccableDashboardMenuVisible=0; //stop sending params request when motor is off
									#endif

									//engine speed fail is on byte1 bit 1.
									//engine StopStart Status is on byte 1 bit 0 and byte 2 bit 7
									//engine Status is on byte 2 bit 6 and bit 5.
									//gas pedal position is on byte 2 from bit 4 to 0 and byte 3 from bit 7 to 5.
									//gas pedal position fail is on byte3 bit 4.
									//.....
									//alternator fail is on byte 3, bit1.
									//stopStart status is on byte3 bit 0 and byte4 bit7.
									//CC brake intervention request is on byte 4, bit5
									//bank deactivation status is on byte5, bit 7 and 6
									//CC brake intervention is on byte 5 from bit 5 to 0 and byte 6 from bit 7 to 4.
									break;
								case 0x000001F0:
									//clutch interlock is on byte 0 bit 7
									//clutch upstop is on byte0 bit 6
									//actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
									//analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.

									break;
								case 0x0000001F7:
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(rx_msg_header.DLC>=4){
											transmissionTemperature= ((rx_msg_data[2] & 0b00000001) << 5 | ((rx_msg_data[3] >> 3) & 0b00011111));
											//onboardLed_blue_on();
										}
									#endif
									//transmission temperature is on byte 2 bit 0 and byte 3 from bit 7 to bit 3
									break;
								case 0x000001FC: //received on C2 can bus
									//Rear Diff. Warning La. is on byte0 bit7
									//Rear Diff, Control Status is on byte0 bit6
									//Active Dumping Control Status (the suspensions) is on byte0 from bit 5 to 4 (0x0=Mid, 0x1=Soft, 0x2=Firm [only on QV])
									//Rd. Asp. Ind. is on byte 0 from bit3 to 0 and byte 1 from bit 7 to 4
									//Active Dumping Control Fail status is on byte 1 bit3
									//Aero. Fail Status is on byte 1 bit2
									//Front Aero. status is on byte 1 bit1 to bit0
									//CDCM warning lamp is on byte 2 bit5
									break;
								case 0x00000226:
									#if (defined(DISABLE_START_STOP) || defined(SMART_DISABLE_START_STOP))
										if(rx_msg_header.DLC>=2){
											//fill a variable with start&stop Status
											if(rx_msg_data[2]==0xF1) startAndstopCarStatus=1; //start&stop enabled in car (fefault in giulias)
											if(rx_msg_data[2]==0x05) startAndstopCarStatus=0; //start&stop disabled in car
										}
									#endif
									break;
								case 0x000002ED: //message to dashboard containing shift indicator
									#if defined(SHIFT_INDICATOR_ENABLED)
										if(rx_msg_header.DLC==8){
											if (currentRpmSpeed>SHIFT_THRESHOLD-1 ){ //if the rpm speed is above SHIFT_THRESHOLD rpm, then the packet need to be modified, therefore,
												//copy 8 bytes from msgdata rx (rx_msg_data) to msg data tx (shift_msg_data)
												memcpy(&shift_msg_data, &rx_msg_data, 8);

												if(currentRpmSpeed>(SHIFT_THRESHOLD-1) &&  currentRpmSpeed<(SHIFT_THRESHOLD+500)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) to 01 (binary) meaning "gear shift urgency level 1"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x1 & 0x3);
												}
												if(currentRpmSpeed>(SHIFT_THRESHOLD+500-1) &&  currentRpmSpeed<(SHIFT_THRESHOLD+1000)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) of the message to 10 (binary) meaning "gear shift urgency level 2"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x2 & 0x3);
												}else if(currentRpmSpeed>(SHIFT_THRESHOLD+1000-1)){ //set lamp depending on rpm speed
													//change byte6, bit 1 and 0(lsb) of the message to 11 (binary) meaning "gear shift urgency level 3"
													shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x3 & 0x3);
												}
												can_tx(&shift_msg_header, shift_msg_data); //transmit the modified packet
												onboardLed_blue_on();
											}

										}
									#endif

										//EngineWaterTemperature is on byte0
										//fuel consumption is on byte 4 bit 0 to 3, byte 5, and byte 6 from bit 7 to 3
									break;
								case 0x000002EE: // presente solo su BH can bus a 125kbps
									//this message contains the following radio buttons on the steering wheel:
									//radio right button is on byte 3 bit6 (1=button pressed)
									//radio left button on the steering wheel is on byte 3 bit4 (1=button pressed)
									//radio Voice command button is on byte 3 bit2 (1= button pressed)
									//phone call button is on byte3 bit0(1=button pressed)
									//volume  is on byte 4 (volume up increases the value, volume down reduces the value. once arrived to 255 restarts from 0 and under 0 goes to 255)
									//volume change is on byte5 bit 7 and bit6 (1=volume was increased rotation, 2=volume decreased rotation, 3=volume mute button press) (then reading the entire byte we will see respectively, 0x40, 0x80,  0xC0)
									//sample: uint8_t tmpCmd=rx_msg_data[5] >>6; //1=volume was increased rotation, 2=volume decreased rotation
									break;
								case 0x000002EF: //se e' il messaggio che contiene la marcia (id 2ef) e se é lungo 8 byte
									#if defined(LED_STRIP_CONTROLLER_ENABLED) || defined(IMMOBILIZER_ENABLED)
										currentGear=rx_msg_data[0] & ~0xF;
									#endif

									#if defined(LED_STRIP_CONTROLLER_ENABLED)
										scaledColorSet=scaleColorSet(currentGear ); //prima di tutto azzeriamo i primi 4 bit meno significativi, poi scala il dato con la funzione scaleColorSet, per prepararlo per l'invio alla classe vumeter
										vuMeterUpdate(scaledVolume,scaledColorSet);
									#endif

									//actual gear status is on byte 0 from bit 7 to 4 (0x0=neutral, 0x1 to 0x6=gear 1 to 6, 0x07=reverse gear, 0x8 to 0xA=gear 7 to 9, 0xF=SNA)
									//suggested gear status is on byte 0 from bit 3 to 0
									//DPF Regeneration mode is on byte 1 bit 7.
									//SAM info is on byte 1 from bit 3 to 0
									//stop start fault status is on byte 2 bit 7
									//..
									//boost pressure indication is on byte 3 bit from 6 to 0 and byte 4  bit 7


									break;
								case 0x000002FA: // Button is pressed on left area of the wheel
									// These Buttons are detected only if the main panel of the car is on.

									//This is used if the SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined

									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(cruiseControlDisabled){ //if we are allowed to press buttons, use them in baccable menu
											switch(rx_msg_data[0]){
												case 0x18://if cruise control speed reduction button was pressed, user wants to see next page
													if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
														wheelPressedButtonID=0x18; //avoid to return here
														dashboardPageIndex += 1; //set next page
														if(dashboardPageIndex>=total_pages_in_dashboard_menu)  dashboardPageIndex=0; // make a rotative menu
														//onboardLed_blue_on();
														sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
													}
													break;
												case 0x20://if cruise control speed strong reduction button was pressed, user wants to jump 10 pages forward
														if(wheelPressedButtonID==0x18 && baccableDashboardMenuVisible){ //if button released, use pressed button
															wheelPressedButtonID=0x20; //avoid to return here
															dashboardPageIndex += 10; //set 9 pages forward (+1 in gentle command)
															if(dashboardPageIndex>=total_pages_in_dashboard_menu)  dashboardPageIndex=0; // make a rotative menu
															//onboardLed_blue_on();
															sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
														}
														break;
												case 0x08: //if cruise control speed increase button was pressed, user wants to see previous page
													if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
														wheelPressedButtonID=0x08; //avoid to enter again here
														dashboardPageIndex -= 1; //set previous page
														if(dashboardPageIndex>=total_pages_in_dashboard_menu)  dashboardPageIndex=total_pages_in_dashboard_menu-1; // make a rotative menu
														//onboardLed_blue_on();
														sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
													}
													break;
												case 0x00: //if cruise control speed strong increase button was pressed, user wants to jump 10 pages before
														if(wheelPressedButtonID==0x08 && baccableDashboardMenuVisible){
															wheelPressedButtonID=0x00; //avoid to return here

															dashboardPageIndex -= 10; //set 10 pages backward
															if(dashboardPageIndex>=total_pages_in_dashboard_menu)  dashboardPageIndex=0; // stay at zero.


															//onboardLed_blue_on();
															sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
														}
														break;
												case 0x10: // button released
													wheelPressedButtonID=0x10; //button released
													lastPressedWheelButtonDuration=0;
													break;
												case 0x90: //RES button was pressed

													lastPressedWheelButtonDuration++;
													if (wheelPressedButtonID==0x10 && (lastPressedWheelButtonDuration>50)){//we pressed RES for around 2 seconds, therefore we want to enable/disable Baccable menu on dashboard
														wheelPressedButtonID=0x90; //avoid returning here until button is not released

														baccableDashboardMenuVisible=!baccableDashboardMenuVisible; //toggle visualizazion of the menu

														if(!baccableDashboardMenuVisible){ //if menu needs to be hidden, print spaces to clear the string on dashboard
															clearDashboardBaccableMenu();
														}else{
															dashboardPageIndex=0; //reset the page, just to be sure to show initial Baccable print
														}


													}
													break;
												case 0x12: //Cruise Control Disabled/Enabled
													break;
												default:
											}
										}
									#endif

									#if defined(IMMOBILIZER_ENABLED)
										if(cruiseControlDisabled){ //if we are allowed to use the buttons of the cruise control
											if (currentRpmSpeed>400){ //if motor is on
												if(currentGear==0){ //gear is neutral
													if((rx_msg_data[0]==0x08) && ((wheelPressedButtonID==0x10) || (wheelPressedButtonID==0x08))){ //user is pressing CC soft speed up button and it was previously released (or pressed by baccable menu up here)
														lastPressedSpeedUpWheelButtonDuration++;
														if(lastPressedSpeedUpWheelButtonDuration>1267){ //around 30 seconds
															//avoid to return here
															wheelPressedButtonID=0xF8; //invent a new status to differentiate it from 0x08 used in baccable menu few lines of code up here
															lastPressedSpeedUpWheelButtonDuration=0; //unuseful here since it is done when button is released. just to be superstitious :-D.
															immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status
															floodTheBus=0; //ensure to reset this even if probably it is not needed
															if(saveOnflash((uint16_t)immobilizerEnabled)>253){ //if we get error while permanently storeing the parameter on flash
																immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status to the original status and avoid to report the user anything
																onboardLed_red_on(); //a problem occurred
															}else{
																onboardLed_blue_on(); //everything goes fine. change saved on flash
																if(immobilizerEnabled){ //if immo enabled
																	executeDashboardBlinks=6; //blinks the dashboard brightness 3 times
																}else{
																	executeDashboardBlinks=12; //blinks the dashboard brightness 6 times
																}
															}


														}
													}
													if(rx_msg_data[0]==0x10){ //user released the button
														lastPressedSpeedUpWheelButtonDuration=0;
														wheelPressedButtonID=0x10; //button released
													}
												}

											}
										}

									#endif

									// We commented this section since we are not using it now. Uncomment to use it (this is old, revise it if required).
									// The cycle duration could increase and make blink the red led onboard.
									// The action of the button's sequence is left empty for you to add it.


									// it shall be different than 0x10 because sometimes is found on the bus,
									// but we are not interested in it.
									/*
									if( (rx_msg_header.DLC==3) && (rx_msg_data[0]!=0x10) ){
										//if 2 seconds has passed since last button press, restart password sequence
										if (lastPressedWheelButtonTime+2000<HAL_GetTick()){
											ButtonPressSequence1Index=0;
											lastPressedWheelButtonDuration=0;
											lastPressedWheelButtonTime=HAL_GetTick();
										}
										//if the previous button is still pressed, we shall consider it as the same button press
										if ((lastPressedWheelButton== rx_msg_data[0]) ){
											lastPressedWheelButtonDuration=lastPressedWheelButtonDuration+ (HAL_GetTick()-lastPressedWheelButtonTime) ;
										}else{
											//another button was pressed, let's consider it as a button release event:
											//  if the button was pressed for at least 100msec, and if it is the button expected by the secret sequence
											if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index])){
												onboardLed_blue_on();
												ButtonPressSequence1Index++; //prepare for next button in the sequence
											}else{
												//something else arrived. restart the sequence
												ButtonPressSequence1Index=0;
											}
											lastPressedWheelButtonDuration=0;
										}

										//manage the particular case of last button of the sequence pressed for enough time
										if(lastPressedWheelButtonDuration>100 && (lastPressedWheelButton==ButtonPressSequence1[ButtonPressSequence1Index]) && ButtonPressSequence1Index==ButtonPressSequence1Len-1){
											ButtonPressSequence1Index++;
										}

										lastPressedWheelButton= rx_msg_data[0]; //store last pressed button
										lastPressedWheelButtonTime=HAL_GetTick(); //store last time a button was pressed

										//if password sequence was fully correctly typed...
										if(ButtonPressSequence1Index==ButtonPressSequence1Len){
											// reset password sequence, so that we can type it again in the future,
											// then do what you shall do
											ButtonPressSequence1Index=0;
											onboardLed_red_on();
											// what do we want to do? this will work only if the bus is not flood
											// and the car panel is on.
											// future usage....
										}
									}
									*/
									break;
								case 0x00000384:
									#if defined(ESC_TC_CUSTOMIZATOR_ENABLED)
										//on C2 can bus, msg 0x384 contains, in byte3, bit6 contains left stalk button press status (LANE indicator button)
										if((rx_msg_data[3] & 0x40) ==0x40){ // left stalk button was pressed (lane following indicator)

											LANEbuttonPressLastTimeSeen=HAL_GetTick();//save current time it was pressed as LANEbuttonPressLastTimeSeen
											LANEbuttonPressCount++;
											if (LANEbuttonPressCount>8 ){ //8 is more or less 2 seconds
												ESCandTCinversion=!ESCandTCinversion; //toggle the status
												#if defined(DYNO_MODE) //if dyno is enabled or its change is in progress, avoid to switch ESP/TC.
													if(DynoModeEnabled || DynoStateMachine!=0xff) ESCandTCinversion=!ESCandTCinversion; //revert the change. won't do both things
												#endif
												onboardLed_blue_on();
												LANEbuttonPressCount=0; //reset the count
											}
										}else{
											if(LANEbuttonPressLastTimeSeen+1000<HAL_GetTick()){ // if LANEbuttonPressLastTimeSeen, is older than 1 second ago, it means that button was released
												LANEbuttonPressCount=0;// reset the count assigning it zero
											}
										}

										if(currentDNAmode!=(rx_msg_data[1]& 0x7C)){ //RDNA mode was changed, reset the ESCandTCinversion
											ESCandTCinversion=0;
										}
										//current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08], 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
										currentDNAmode=rx_msg_data[1] & 0x7C; //7C is the mask from bit 6 to 2
										if (ESCandTCinversion){
											memcpy(&DNA_msg_data, &rx_msg_data, 8);
											if(currentDNAmode==0x30){ //race
												DNA_msg_data[1]= (DNA_msg_data[1] & ~0x7C) | (0x08 & 0x7C); //set Dynamic mode (0x08) to enable ESC and TC
											}else{
												DNA_msg_data[1] = (DNA_msg_data[1] & ~0x7C) | (0x30 & 0x7C);  //set Race mode (0x30) to disable ESC and TC
											}
											can_tx(&DNA_msg_header, DNA_msg_data); //transmit the modified packet
											//onboardLed_blue_on();
										}
									#endif

																		//Command Ignition Status is on byte0 from bit 3 to 1.
									//Command Ignition Fail Status is on byte 0 bit0 and in byte1 bit7.
									//Drive Style Status (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
									//External temperature is on byte 1 from bit 1 to 0 and on byte 2 from bit 7 to bit 1.
									//External temperature fail is on byte2 bit0
									//Low Beam Status is on byte3 bit7
									//Lane Indicator button status (left stalk button) is on byte 3 bit6.
									//Power Mode Status is on byte 3 from bit 5 to 4.
									//Park Brake Status is on byte 3 bit 3.
									//Int. Relay Fail Status is on byte 4 from bit 7 to 6
									//SuspensionLevel is on byte 5 bit0 and byte 6 bit7.

									break;
								case 0x00000412: //se e' il messaggio che contiene la pressione dell'acceleratore (id 412), se é lungo 5 byte, se il valore é >51 (sfrutto le info ottenute sniffando)
									#if defined(LED_STRIP_CONTROLLER_ENABLED)
										if( (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51) ){
											timeSinceLastReceivedAcceleratorMessage=HAL_GetTick();
											ledsStripIsOn=1;
											scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
											vuMeterUpdate(scaledVolume,scaledColorSet);
										}
									#endif
									break;
								case 0x0000041A:
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(rx_msg_header.DLC>=6){
											batteryStateOfCharge= (rx_msg_data[1] & 0b01111111); //set Most Significant Bit to zero
											batteryCurrent= (rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111));
										}
									#endif
									//battery state of charge is on byte 1 from bit 6 to 0 (Percentage)
									//battery current (A) is on byte 4 and in byte 5 from bit 7 to bit 4
									break;
								case 0x000004B1:
									#if defined(SMART_DISABLE_START_STOP)
										//grab the message
										if(rx_msg_header.DLC==8){
											memcpy(&disableStartAndStopMsgData, &rx_msg_data, 8);
										}
									#endif
									// Bonnet Status is on byte0 bit 4
									// driver door Fail status is on byte0 bit 3
									// FOB Search Request is on byte 0 bit from 2 to 1
									// Driver door status is on byte 0 bit 0
									//Passenger Door status is on byte1, bit 7
									//Left  Rear Door status is on byte 1 bit 6
									//Right Rear Door status is on byte 1 bit 5
									//Rear Hatch Status is on byte 1 bit 4
									//Rear Heated Window Status is on byte 1 bit 3
									//Front Heated Window Status is on byte 1 bit 2
									//Theft Alarm Status is on byte 2 from bit 6 to 4
									//Remote start Inhibit Status is on byte 2 from bit 3 to 0 and byte 3 from bit 7 to 6
									//Remote start Active status is on byte 3 bit 5
									//Battery state of function is on byte 3 from bit 4 to 0 and byte 4 bit 7
									//compressor Air Conditioner status is on byte 4 bit 5
									//Recalibration is on byte 4 bit 3
									//Exterior Rear Release Switch Status is on byte 4 bit 1
									//Start&Stop Pad1 is on byte 5 from bit 5 to 3 (value 1 enables and disables Start& stop)

									break;
								case 0x000004B2:
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
										if(rx_msg_header.DLC>=4){
											oilPressure= ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111));
											oilTemperature= ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011));
										}


									#endif
									//engine oil level is in byte 0 from bit 7 to 3.
									//engine oil over fill status is on byte 0, bit 2.
									//engine oil min. is on byte 0 bit 1
									//engine oil pressure is on byte 0, bit 0 and on byte 1 from bit 7 to 1. (bar)
									//power mode status is on byte 1 bit 0 and on byte 2 bit 7.
									//engine water level is on byte 2 bit 6.
									//engine oil temperature is on byte 2 from bit 5 to 0 and on byte 3 from bit 7 to 6.
									//engine oil temperature warning light is on byte 3 bit 5.
									break;
								case 0x00000545:
									#if defined(IMMOBILIZER_ENABLED)
										if(rx_msg_header.DLC==8){
											memcpy(&dashboardBlinkMsgData, &rx_msg_data, 8);
										}
									#endif
									//only if  lights are ON, and therefore the dashboard is  set to max brightness: setting byte 5 to 0x00, the brightness increases for around 100msec (this works for any value between 0x and 7x )
									//only if lights are OFF, and therefore the dashboard is set to min brightness: setting byte 5 to 0xF0, the brightness reduces for around 100msec (this works for any value between Dx and Fx)
									//this is the test message to increase brightness: 0x88 0x20 0xC3 0x24 0x00 0x14 0x30 0x00
									break;
								case 0x000005A5:
									//cruise control ON/OFF status is on byte0 bit7 (0=disabled, 1=enabled)
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED)
										if((rx_msg_data[0]>>7)==1){
											cruiseControlDisabled=0;//disable additional parameter menu commands
										}else{
											cruiseControlDisabled=1;//enable additional parameter menu commands
										}
									#endif
									break;
								case 0x000005B0:
									#if defined(DYNO_MODE)
										if((rx_msg_data[1] == 0x20) && ( DynoStateMachine == 0xff)){ // park assist button was pressed and there is no dyno Start sequence in progress
											ParkAssistButtonPressCount++;
											if (ParkAssistButtonPressCount>5){ // more or less 6 seconds
												ParkAssistButtonPressCount=0; //reset the count
												DynoStateMachine=0; //state machine
												#if defined(ESC_TC_CUSTOMIZATOR_ENABLED)
														ESCandTCinversion=0; //do not change ESC and TC if dynomode is requested
												#endif
												DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1; //length of DIAGNOSTIC SESSION msg
												can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
												onboardLed_blue_on();
												DynoStateMachineLastUpdateTime=HAL_GetTick();//save last time seen

												//wait the feedback from ECU
											}
										}else{
											ParkAssistButtonPressCount=0;// reset the count assigning it zero
										}
									#endif
									//the park assistant button press event is on byte 1 bit 5 (1=pressed)
									break;
								case 0x0000073A:
									//contains current date from byte 0 to 7.
									//Hex values are used as characters in example 0x21 0x02 0x26 0x01 0x20 0x 25 represents
									//the date h21 minutes 02 day 26 month 01 year 2025.
									//last two bytes of the message are 00 00.
									break;
								case 0x0000073C:
									#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(IMMOBILIZER_ENABLED)
										if(rx_msg_header.DLC>=8){
											if(((rx_msg_data[7]>>4) & 0x07) ==0){
												cruiseControlDisabled=1;//enable additional parameter menu commands
											}else{
												cruiseControlDisabled=0;//disable additional parameter menu commands
											}
										}
									#endif
									//contains status of ACC on byte 7, from bit 6 to 4 (0=disabled, 1=enabled, 2=engaged 3=engaged brake only, 4=override, 5=cancel)
								default:
							}

							break;
						default:
					}
				}
			}
		}

		//for debug, measure the loop duration
		if (HAL_GetTick()>debugTimer0+2){
			onboardLed_red_on();
		}
	}
}

#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	void sendDashboardPageToSlaveBaccable(float param){
		uint8_t tmpStrLen=0;
		uint8_t tmpStrLen2=0;
		uint8_t tmpStrLen3=0;

		uartTxMsg[0]= BhBusID;//first char shall be a # to talk with slave canable connected to BH can bus

		switch(uds_params_array[dashboardPageIndex].reqId){ //do preliminary additional stuff for special parameters (not uds)
			case 0x10: //print oil pressure
				param=(float) oilPressure * uds_params_array[dashboardPageIndex].replyScale;
				break;
			case 0x11: //power in CV
				param=((float)torque + uds_params_array[dashboardPageIndex].replyValOffset) * (float)currentRpmSpeed * uds_params_array[dashboardPageIndex].replyScale ;
				break;
			case 0x12: //torque in NM
				param=((float)torque  * uds_params_array[dashboardPageIndex].replyScale) + uds_params_array[dashboardPageIndex].replyScaleOffset;
				break;
			case 0x13: //battery state of charge (%)
				param=(float)batteryStateOfCharge  * uds_params_array[dashboardPageIndex].replyScale;
				break;
			case 0x14: //battery current (A)
				param=((float)batteryCurrent  * uds_params_array[dashboardPageIndex].replyScale) + uds_params_array[dashboardPageIndex].replyScaleOffset;
				break;
			case 0x15: //engine oil temperature
				param=((float)oilTemperature * uds_params_array[dashboardPageIndex].replyScale) + uds_params_array[dashboardPageIndex].replyScaleOffset;
				break;
			case 0x16: //transmission temperature
				param=((float)transmissionTemperature * uds_params_array[dashboardPageIndex].replyScale) + uds_params_array[dashboardPageIndex].replyScaleOffset;
			default:
				break;
		}


		switch(uds_params_array[dashboardPageIndex].reqId){
			case 0: //print baccable menu
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>18) tmpStrLen=18;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				break;
			default:
				tmpStrLen=strlen((const char *)uds_params_array[dashboardPageIndex].name);
				if(tmpStrLen>18) tmpStrLen=18; //truncate it. no space left
				memcpy(&uartTxMsg[1], &uds_params_array[dashboardPageIndex].name,tmpStrLen); //prepare name of parameter
				if(param!=-3400000000000000000){ //if different than special value (since special value means no value to send)
					//scale param still done, we don't need to do it here
					//param += uds_params_array[dashboardPageIndex].replyValOffset;
					//param *= uds_params_array[dashboardPageIndex].replyScale;
					//param += uds_params_array[dashboardPageIndex].replyScaleOffset;
					//convert param from float to string
					char tmpfloatString[10];
					floatToStr(tmpfloatString,param,uds_params_array[dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
					//add param to the page String
					tmpStrLen2=strlen(tmpfloatString);
					if(tmpStrLen+tmpStrLen2>18) tmpStrLen2=18-tmpStrLen; //truncate it. no space left
					memcpy(&uartTxMsg[1+tmpStrLen],tmpfloatString,tmpStrLen2);

					//float tmpVal9=200000.45601928209374; ///ADDED FOR TEST.......
					//char *tmpStr9=(char*)malloc(10);

					//floatToStr(tmpfloatString,param,2,sizeof(tmpfloatString));
					//tmpStrLen2=strlen(tmpfloatString);
					//memcpy(&dashboardPageStringArray[tmpStrLen],tmpfloatString,tmpStrLen2);
				}
				//add measurement unit
				tmpStrLen3=strlen((const char *)uds_params_array[dashboardPageIndex].replyMeasurementUnit);
				if(tmpStrLen+tmpStrLen2+tmpStrLen3>18) tmpStrLen3=18-tmpStrLen-tmpStrLen2; //truncate it. no space left
				memcpy(&uartTxMsg[1+tmpStrLen+tmpStrLen2],&uds_params_array[dashboardPageIndex].replyMeasurementUnit,tmpStrLen3);
		}
		if (tmpStrLen+tmpStrLen2+tmpStrLen3 < 18) { //if required pad with zeros
			memset(&uartTxMsg[1+tmpStrLen+tmpStrLen2+tmpStrLen3], 0, UART_BUFFER_SIZE-(1+tmpStrLen+tmpStrLen2+tmpStrLen3)); //set to zero remaining chars
		}
		//CDC_Transmit_FS(dashboardPageStringArray, tmpStrLen+tmpStrLen2+tmpStrLen3+1); //send it over usb
		if( __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC)){
			if (HAL_UART_Transmit_IT(&huart2, uartTxMsg, UART_BUFFER_SIZE) != HAL_OK){ //send it asyncronously (with interruot) over uart
				//onboardLed_red_on();
				Error_Handler(); //manage error in case of fail
			}
		}

	}
#endif
// this function scales value received from can bus. It is assumed that pedal position (or motor rpm) will change vumeter volume represented with the leds strip
float scaleVolume(uint8_t vol){
	//Scale this value to get a percentage between 0 and 100
	//sniffed data on can bus: msg id=0x412 , fourth byte goes from 0x33(51) to 0xE6(230)
	if (vol>50){
		vol=vol-51;
	}
	return (float)(((float)vol*100.0f)/180.0f);
}

// this function scales value received from can bus. It is assumed that gear selection will change the color effect of the leds strip
uint8_t scaleColorSet(uint8_t col){
	//id 2ef, primo byte, 70=r, 00=n, f0=marcia inserita ma frizione premuta (indefinito), 10=prima, 20=seconda..
	col= col>>4;
	//onboardLed_red_blink(col);
	// 7=backward, f=gear set but frizione premuta (undefined), 1=first gear , 2=second gear, ... , 6= sixt gear
	return col;
}

uint8_t saveOnflash(uint16_t param1){ //store params permanently on flash
	//last page to store on flash is 0x0801 F800 (we can store 2 bytes each time)
	// and we shall erase entire page before write. one page size is FLASH_PAGE_SIZE (2048 bytes in st32F072)
	HAL_FLASH_Unlock(); //unlock flash

	//erase flash
	FLASH_EraseInitTypeDef eraseInitStruct;
	uint32_t pageError=0;
	eraseInitStruct.TypeErase= FLASH_TYPEERASE_PAGES;
	eraseInitStruct.PageAddress=LAST_PAGE_ADDRESS; //last page address begin
	eraseInitStruct.NbPages=1;
	if(HAL_FLASHEx_Erase(&eraseInitStruct,&pageError)!=HAL_OK){ //error during erase
		HAL_FLASH_Lock();
		onboardLed_red_blink(8);
		return 254; //error
	}

	//write parameter
	if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,LAST_PAGE_ADDRESS,param1)!=HAL_OK){ //error during write
		HAL_FLASH_Lock();
		onboardLed_red_blink(9);
		return 255; //error
	}

	//lock the flash
	HAL_FLASH_Lock();
	return 0;
}

uint16_t readFromFlash(uint8_t paramId){
	switch(paramId){
	case 1: //immobilizer enable status (1=enabled 0=disabled)
		return !(!( *((uint16_t*)LAST_PAGE_ADDRESS) )); //double negation to coerce to a boolean even when it is FFFF. default FFFF will mean immo enabled
		break;
	default:
		return 0;
		break;
	}

}
#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	void clearDashboardBaccableMenu(){
		//prepare empty message
		uartTxMsg[0]=BhBusID; //# to send message to baccable slave connected to BH can bus
		for(uint8_t i=1;i<UART_BUFFER_SIZE;i++){
			uartTxMsg[i]=0x20; //space char
		}
		//send it
		//CDC_Transmit_FS(uartTxMsg, UART_BUFFER_SIZE); //send it over usb
		if( __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC)){
			if (HAL_UART_Transmit_IT(&huart2, uartTxMsg, UART_BUFFER_SIZE) != HAL_OK){ //send it asyncronously (with interruot) over uart
				//onboardLed_red_on();
				Error_Handler(); //manage error in case of fail
			}
		}
	}
#endif

void floatToStr(char* str, float num, uint8_t precision, uint8_t maxLen) {
    uint8_t i = 0;

    // Gestione dei casi speciali NaN e Inf
    if (num != num) {  // NaN check
        if (maxLen > 3) {
            str[0] = 'N'; str[1] = 'a'; str[2] = 'N'; str[3] = '\0';
        }
        return;
    }
    if (num == (float)INFINITY) {
        if (maxLen > 3) {
            str[0] = 'I'; str[1] = 'n'; str[2] = 'f'; str[3] = '\0';
        }
        return;
    }
    if (num == (float)-INFINITY) {
        if (maxLen > 4) {
            str[0] = '-'; str[1] = 'I'; str[2] = 'n'; str[3] = 'f'; str[4] = '\0';
        }
        return;
    }

    // Calcolo del fattore di arrotondamento corretto
    float roundingFactor = 0.5f;
    for (uint8_t j = 0; j < precision; j++) {
        roundingFactor /= 10.0f;
    }
    if (num < 0) {
        num -= roundingFactor;  // Arrotondamento corretto per numeri negativi
    } else {
        num += roundingFactor;  // Arrotondamento per numeri positivi
    }

    // Gestione del segno
    if (num < 0) {
        if (i < maxLen - 1) {
            str[i++] = '-';
        }
        num = -num;
    }

    // Parte intera e parte decimale
    uint32_t intPart = (uint32_t)num;
    uint32_t scale = 1;
    for (uint8_t j = 0; j < precision; j++) {
        scale *= 10;
    }
    uint32_t decPart = (uint32_t)((num - intPart) * scale);

    // Conversione della parte intera
    uint8_t intStart = i;
    if (intPart == 0) {
        if (i < maxLen - 1) {
            str[i++] = '0';
        }
    } else {
        uint8_t count = 0;
        uint32_t tmp = intPart;
        while (tmp > 0) {
            tmp /= 10;
            count++;
        }
        for (uint8_t j = count; j > 0; j--) {
            if (i < maxLen - 1) {
                str[i + j - 1] = (intPart % 10) + '0';
            }
            intPart /= 10;
        }
        i += count;
    }

    // Conversione della parte decimale
    if (precision > 0 && i < maxLen - 1) {
        str[i++] = '.';
        for (uint8_t j = 0; j < precision; j++) {
            if (i < maxLen - 1) {
                decPart *= 10;
                str[i++] = (decPart / scale) + '0';
                decPart %= scale;
            }
        }
    }

    // Rimuovere zeri finali superflui
    if (precision > 0) {
        while (i > intStart && str[i - 1] == '0') {
            str[--i] = '\0';
        }
        if (i > intStart && str[i - 1] == '.') {
            str[--i] = '\0';
        }
    }

    // Aggiungere terminatore di stringa
    if (i < maxLen) {
        str[i] = '\0';
    } else if (maxLen > 0) {
        str[maxLen - 1] = '\0';
    }
}

//System Clock Configuration
void SystemClock_Config(void){
  HAL_Init();
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK){
    Error_Handler();
  }

  //the following part is used by usb, used by canable

  // Set USB clock source to HSI48 (48 MHz)
  RCC_PeriphCLKInitTypeDef PeriphClkInit= {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
	  Error_Handler();
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

void Error_Handler(void){
  __disable_irq();
  while (1){}
}

// Disable all interrupts
void system_irq_disable(void){
	__disable_irq();
	__DSB();
	__ISB();
}


// Enable all interrupts
void system_irq_enable(void){
        __enable_irq();
}

void system_hex32(char *out, uint32_t val){
	char *p = out + 8;
	*p-- = 0;
	while (p >= out) {
		uint8_t nybble = val & 0x0F;
		if (nybble < 10)
			*p = '0' + nybble;
		else
			*p = 'A' + nybble - 10;
		val >>= 4;
		p--;
	}
}


#ifdef  USE_FULL_ASSERT
	/**
	  * @brief  Reports the name of the source file and the source line number
	  *         where the assert_param error has occurred.
	  * @param  file: pointer to the source file name
	  * @param  line: assert_param error line source number
	  * @retval None
	  */
	void assert_failed(uint8_t *file, uint32_t line){
	  /* USER CODE BEGIN 6 */
	  /* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	  /* USER CODE END 6 */
	}
#endif /* USE_FULL_ASSERT */
