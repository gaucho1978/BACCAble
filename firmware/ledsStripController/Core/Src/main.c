// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"

#if defined(ACT_AS_CANABLE)
	#include "usb_device.h"
	#include "usbd_cdc_if.h"
	#include "string.h"
#endif

#include "uart.h"

#if defined(C1baccable)
	#include "vuMeter.h" //this is used to control led strip through usb pin

	#include "lowConsume.h"
	extern uint32_t lastReceivedCanMsgTime;
#endif

//this is used to store FW version, also shown on usb when used as slcan
#ifndef BUILD_VERSION //compile time define with -D
#define BUILD_VERSION "v2.5.4"
#endif
#define FW_PREFIX "BACCAble "
#define _FW_VERSION FW_PREFIX BUILD_VERSION
const char *FW_VERSION=_FW_VERSION;

// force print
#pragma message ("FW_VERSION: " _FW_VERSION)

#if defined(UCAN_BOARD_LED_INVERSION)
	const uint8_t led_light_on_bit=1;
#else
	const uint8_t led_light_on_bit=0;
#endif


uint16_t indiceTmp=33;


#if defined(C1baccable)

	uint32_t shutdownDashboardMenuRequestTime=0; //used to shutdown display after one minute from motor off event

	uint8_t checkbox_symbols[2] = {0x4F, 0xD8}; // O (0x4F= not selected), Ø (0xD8= selected)

	//ACC_VIRTUAL_PAD
	uint8_t function_acc_virtual_pad_enabled=0; //default disabled. saved in flash
	//function Virtual ACC Pad
	CAN_TxHeaderTypeDef ACC_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2FA, .DLC=3};
	uint8_t ACC_msg_data[3];
	uint8_t newWheelPressedButtonID=0x10; //button released
	uint8_t ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=0;

	//IMMOBILIZER_ENABLED
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

	//SMART_DISABLE_START_STOP
	//DISABLE_START_STOP
	uint8_t function_smart_disable_start_stop_enabled=1; //added to manage the function from the setup menu on the dashboard
	CAN_TxHeaderTypeDef disableStartAndStopMsgHeader={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x4B1, .DLC=8};
	uint8_t disableStartAndStopMsgData[8]= {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; //byte 5 shall be set to 0x08
	uint8_t startAndStopEnabled=1; //this is the status of my internal logic. If=0 the function goes to sleep up to next reboot
	uint8_t startAndstopCarStatus=1; //this is the status of Start&stop received by the car. 1=enabled in car (this is the default status in giulias).
	uint32_t lastTimeStartAndstopDisablerButtonPressed=0;
	uint8_t requestToDisableStartAndStop=0; //if set to 1 sends message simulating s&s button press

	//
	uint8_t gearArray[11]={'N','1','2','3','4','5','6','R','7','8','9'};

	uint8_t main_dashboardPageIndex=0;
	uint8_t dashboard_menu_indent_level=0;
	uint8_t dashboard_main_menu_array_len=10;
	uint8_t dashboard_main_menu_array[10][DASHBOARD_MESSAGE_MAX_LENGTH]={
			{},
			{'S','H','O','W',' ','P','A','R','A','M','S',' ',' ',' ',' ',' ',' ',' '},
			{'R','E','A','D',' ','F','A','U','L','T','S',' ',' ',' ',' ',' ',' ',' '},
			{'C','L','E','A','R',' ','F','A','U','L','T','S',' ',' ',' ',' ',' ',' '},
			{'I','m','m','o','b','i','l','i','z','e','r',' ',' ','O','N',' ',' ',' '},
			{'T','o','g','g','l','e',' ','D','Y','N','O',' ',' ',' ',' ',' ',' ',' '},
			{'T','o','g','g','l','e',' ','E','S','C','/','T','C',' ',' ',' ',' ',' '},
			{'F','r','o','n','t',' ','B','r','a','k','e',' ','N','o','r','m','a','l'},
			{'4','W','D',' ',' ','E','n','a','b','l','e','d',' ',' ',' ',' ',' ',' '},
			{'S','E','T','U','P',' ','M','E','N','U',' ',' ',' ',' ',' ',' ',' ',' '},

	};

	uint8_t setup_dashboardPageIndex=0;
	uint8_t total_pages_in_setup_dashboard_menu=19;
	uint8_t dashboard_setup_menu_array[25][DASHBOARD_MESSAGE_MAX_LENGTH]={
			{'S','A','V','E','&','E','X','I','T',' ',' ',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','S','t','a','r','t','&','S','t','o','p',' ',' ',' ',' ',' '},
			{'O',' ',' ','-','-','-','-','-','-','-','-','-','-','-',' ',' ',' ',' '},
			{'O',' ',' ','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',' '},
			{'O',' ',' ','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
			{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',' ',' ',' ',' '},
			{'O',' ',' ','M','y','2','3',' ','I','P','C',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
			{'O',' ',' ','-','-','-','-','-','-','-','-','-','-','-',' ',' ',' ',' '},
			{'O',' ',' ','R','o','u','t','e',' ','M','e','s','s','a','g','e','s',' '},
			{'O',' ',' ','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',' '},
			{'O',' ',' ','D','y','n','o',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
			{'O',' ',' ','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
			{'O',' ',' ','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
			{'O',' ',' ','4','W','D',' ','D','i','s','a','b','l','e','r',' ',' ',' '},
			{'O',' ',' ','C','l','e','a','r',' ','F','a','u','l','t','s',' ',' ',' '},
			{'O',' ',' ','R','e','a','d',' ',' ','F','a','u','l','t','s',' ',' ',' '},
			{'O',' ',' ','R','e','m','o','t','e',' ','S','t','a','r','t',' ',' ',' '},
			{0xD8,' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
		};

		uint8_t function_is_diesel_enabled=1; //stored in flash. defines if we use gasoline (0) or diesel (1) params
		uint8_t total_pages_in_dashboard_menu_diesel=34;
		uint8_t total_pages_in_dashboard_menu_gasoline=33;
		// uds_params_array[0] contais gasoline params, , uds_params_array[1] contains diesel params
		const	uds_param_element uds_params_array[2][60]={
										{
												{.name={'P','O','W','E','R',':',' ',},								.reqId=0x11,	    .reqLen=4,	.reqData=SWAP_UINT32(0x00000000),   .replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-500,	.replyScale=0.000142378,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'C','V',}						}, //devo ricordare di moltiplicare il risultato per RPM
												{.name={'T','O','R','Q','U','E',':',' ',},							.reqId=0x12,	    .reqLen=4,	.reqData=SWAP_UINT32(0x00000000),   .replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=-500,	.replyDecimalDigits=0,	.replyMeasurementUnit={'N','m',}						},
												{.name={'I','C',' ','A','i','r','O','u','t',':',' '},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221935),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, //TEMPERATURA ARIA uscita INTERCOOLER
												{.name={'I','C',' ','A','i','r','I','n',':',' '},					.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223A58),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, //TEMPERATURA ARIA ingresso INERCOOLER
												{.name={'B','O','O','S','T',' ','A','B','S',':',' '},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322195A),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=-1,     .replyScale=0.001,          .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'B','A','R'}                     }, //PRESSIONE ASSOLUTA
												{.name={'B','O','O','S','T',':',' '},								.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322195A),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=-1,     .replyScale=0.001,          .replyScaleOffset=-1,   .replyDecimalDigits=1,  .replyMeasurementUnit={'B','A','R'}                     }, // PRESSIONE TURBO RICAVATA DALLA PRESIONE ASSOLUTA
												{.name={'T','U','R','B','O',':',},									.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221936),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.0001,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'V',}							},
												{.name={'O','D','O','M','.','L','A','S','T',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222002),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
												{.name={'O','I','L',':',' ',},										.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223A41),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.001,          .replyScaleOffset=0,    .replyDecimalDigits=3,   .replyMeasurementUnit = {'L',},                        }, //QUANTITA' OLIO
												{.name={'O','I','L',':',' ',},										.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322130A),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.039215686,    .replyScaleOffset=0,    .replyDecimalDigits=2,  .replyMeasurementUnit={'B','a','r',}                    },
												{.name={'O','I','L',':',' ',},										.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221302),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=1, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={0xB0,'C',}                       },
												{.name={'O','I','L',' ','Q','U','A','L','Y',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223813),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
												{.name={'O','I','L',' ','U','n','A','i','r',':',' ',},              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322198E),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=-40,    .replyScale=0.02,           .replyScaleOffset=0,    .replyDecimalDigits=2,  .replyMeasurementUnit={0xB0,'C',}                       }, //Temperatura olio mel modulo multiair
												{.name={'G','E','A','R','B','O','X',':',' '},                       .reqId=0x18DA18F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032204FE),   .replyId=0x18DAF118,    .replyLen=1,    .replyOffset=0, .replyValOffset=-40,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        },
												{.name={'B','A','T','T','.',':',},									.reqId=0x13, 		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
												{.name={'B','A','T','T','.',':',},									.reqId=0x14,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-250,	.replyDecimalDigits=2,	.replyMeasurementUnit={'A',}							},
												{.name={'B','A','T','T','.',':',},									.reqId=0x18DA40F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221004),   .replyId=0x18DAF140,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.1,            .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'V',}                            },
			//									{.name={'T', 'Y', 'R', 'E', ' ', 'R', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B3),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
			//									{.name={'T', 'Y', 'R', 'E', ' ', 'R', 'R', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022230B4),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
			//									{.name={'T', 'Y', 'R', 'E', ' ', 'L', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B2),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
			//									{.name={'T', 'Y', 'R', 'E', ' ', 'L', 'F', ':', ' ',},              .reqId=0x18DAC7F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x022240B1),   .replyId=0x18DAF1C7,    .replyLen=1,    .replyOffset=4, .replyValOffset=-50,    .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C'}                        }, // LIMITE BYTE
												{.name={'A','I','R',' ','C','O','N','D','.',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
												{.name={'C','U','R','.',' ','G','E','A','R',':',' ',},				.reqId=0x17,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x000002EF,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
												{.name={'T','-','O','N',':',' '},                                   .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221009),   .replyId=0x18DAF110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.25,           .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={'m',}                            }, // TEMPO TRASCORSO DALL'ACCENZIONE
			// unit grams/km (wrong?)			{.name={'P','A','R','T','I','C','U','L','.', ':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218AA),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0,001,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'g','/','k','m'}					}, // GRAMMI PER KM
												{.name={'O','V','E','R',' ','R','P','M',' ',':',' '},               .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03222006),   .replyId=0xDA18F110,    .replyLen=2,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.01,           .replyScaleOffset=0,    .replyDecimalDigits=2,  .replyMeasurementUnit={'s',}                            }, // PER QUANTO TEMPO SI è ANDATI FUORI RPM
												{.name={'E','X','A','U','S','T',' ','G','A','S',':',' '},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218BA),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=4.52,           .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={0xB0,'C',}                       }, // TEMPERATURA GAS DI SCARICO
												{.name={'C','A','T','A','L','.',':',' '},                                       .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221837),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=-50,    .replyScale=5,              .replyScaleOffset=0,    .replyDecimalDigits=1,  .replyMeasurementUnit={0xB0,'C',}                       }, // TEMPERATURA SONDA CATALIZZATORE
												{.name={'W','A','T','E','R',':',' ',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221003),	.replyId=0x18DAF110,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,			    .replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						}, // TEMPERATURA LIQUIDO REFRIGERANTE
												{.name={'K','N','O','C','K',':',' ',},                              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221841),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.125,          .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'m','V',}                        }, // BATTITO IN TESTA
												{.name={'K','E','Y',' ','I','G','N',':',' ',},                      .reqId=0x18DA40F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03220131),   .replyId=0x18DAF140,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={}                                }, //ID della chiave inserita
												{.name={'O','V','E','R',' ','R','P','M',':',' ',},	                .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03222004),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={}                                }, // NUMERO DI VOLTE CHE SI è ANDATI FUORI RPM
												{.name={'S','P','A','R','K','C','Y','L','1',':',' ',},              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322186C),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}                    }, // CORREZZIONE CILINDRO
												{.name={'S','P','A','R','K','C','Y','L','2',':',' ',},              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322186D),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}                    }, // CORREZZIONE CILINDRO
												{.name={'S','P','A','R','K','C','Y','L','3',':',' ',},              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322186E),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}                    }, // CORREZZIONE CILINDRO
												{.name={'S','P','A','R','K','C','Y','L','4',':',' ',},              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322186F),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=0.0625,         .replyScaleOffset=0,    .replyDecimalDigits=3,  .replyMeasurementUnit={'d','e','g',}                    }, // CORREZZIONE CILINDRO
												{.name={'R','-','D','N','A',':',' ',},                              .reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218F0),   .replyId=0x18DAF110,    .replyLen=1,    .replyOffset=0, .replyValOffset=0,      .replyScale=1,              .replyScaleOffset=0,    .replyDecimalDigits=0,  .replyMeasurementUnit={}                                }, // DATI POSIZIONE R-DNA/DNA
												{.name={'S','P','E','E','D',':',},									.reqId=0x18,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000101,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0625,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'k','m','/','h', }				},
										},
										{
											{.name={'P','O','W','E','R',':',' ',},								.reqId=0x11,	.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),		.replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=-500,	.replyScale=0.000142378,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'C','V',}						}, //devo ricordare di moltiplicare il risultato per RPM
											{.name={'T','O','R','Q','U','E',':',' ',},							.reqId=0x12,	.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),		.replyId=0x000000FB,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=-500,	.replyDecimalDigits=0,	.replyMeasurementUnit={'N','m',}						},
											{.name={'D','P','F',':',' ',},										.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032218E4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
											{.name={'D','P','F',':',' ',},										.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218DE),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
											{.name={'D','P','F',' ','R','E','G','E','N',':',' ', },				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322380B),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.001525902,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
											{.name={'L','A','S','T',' ','R','E','G','E','N','.',':',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223807),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
											{.name={'T','O','T',' ','R','E','G','E','N',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218A4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
											{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223809),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
											{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322380A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01666666666,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','i','n',}					},
											{.name={'R','E','G','E','N',':',' ',' ',' ',' ',' ',' ',},			.reqId=0x19,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000005AE,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
											{.name={'B','A','T','T','.',':',},									.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03221955),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0005,			.replyScaleOffset=0,	.replyDecimalDigits=3,	.replyMeasurementUnit={'V',}							},
											{.name={'B','A','T','T','.',':',},									.reqId=0x13, 		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
											{.name={'B','A','T','T','.',':',},									.reqId=0x14,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x0000041A,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=-250,	.replyDecimalDigits=2,	.replyMeasurementUnit={'A',}							},
											{.name={'O','I','L',' ','Q','U','A','L','Y',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223813),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.0015259022,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'%',}							},
											{.name={'O','I','L',':',' ',},										.reqId=0x15,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000004B2,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=1,				.replyScaleOffset=-40,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
											{.name={'O','I','L',':',' ',},										.reqId=0x10,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000004B2,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,  	.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
											{.name={'C','U','R','.',' ','G','E','A','R',':',' ',},				.reqId=0x17,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x000002EF,	.replyLen=1,	.replyOffset=0, .replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
											{.name={'W','A','T','E','R',':',' ',},								.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221003),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
			//								{.name={'F','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B1),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
			//								{.name={'F','-','R',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B2),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
			//								{.name={'R','-','L',' ','T','I','R','E',':',' ',},					.reqId=0x18DAC7F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x032240B3),	.replyId=0x18DAF1C7,	.replyLen=1,	.replyOffset=4, .replyValOffset=-50,	.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={0xB0,'C',}						},
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
											{.name={'R','A','I','L',':',' ',},									.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221947),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
											{.name={'D','I','E','S','E','L',':',' ',},							.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221900),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
											{.name={'O','D','O','M','.','L','A','S','T',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03222002),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
											{.name={'A','I','R',' ','C','O','N','D','.',':',' ',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322192F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01,			.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'b','a','r',}					},
			//may be not received			{.name={'F','U','E','L',':',' ',},									.reqId=0x18DB33F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03220123),	.replyId=0x18DBF133,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=10,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','P','a',}					},
											{.name={'F','U','E','L',' ','C','O','N','S','.',':',},				.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x03221942),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0000394789,	.replyScaleOffset=0,	.replyDecimalDigits=1,	.replyMeasurementUnit={'L','/','h',}					},
											{.name={'D','E','B','I','M','E','T','E','R',':',},					.reqId=0x18DA10F1,	.reqLen=4,	.reqData=SWAP_UINT32(0x0322193F),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.02,			.replyScaleOffset=-40,	.replyDecimalDigits=1,	.replyMeasurementUnit={0xB0,'C',}						},
											{.name={'S','P','E','E','D',':',},									.reqId=0x18,		.reqLen=4,	.reqData=SWAP_UINT32(0x00000000),	.replyId=0x00000101,	.replyLen=2,	.replyOffset=0, .replyValOffset=0,		.replyScale=0.0625,			.replyScaleOffset=0,	.replyDecimalDigits=2,	.replyMeasurementUnit={'k','m','/','h', }				},

										}

		}; // initializes all the uds parameters request to send and reply to receive

	CAN_TxHeaderTypeDef uds_parameter_request_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA10F1, .DLC=3}; //used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
	uint8_t baccableDashboardMenuVisible=0;
	uint8_t baccabledashboardMenuWasVisible=0; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
	uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
	uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
	uint16_t torque; //torque
	uint8_t batteryStateOfCharge=0; //battery charge %
	uint16_t batteryCurrent; //battery current (to be converted in Amps)
	uint8_t transmissionTemperature;
	uint8_t uds_parameter_request_msg_data[8];//used when SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
	uint8_t dashboardPageIndex=0; //to send message index - it changes when you press cruise control buttons - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality.
	uint32_t last_sent_uds_parameter_request_Time=0; //stores last time we send a uds parameter request - Used with SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE define functionality

	uint8_t dieselEngineMode=0; //0=normal, 1=DPF_REGEN_LO, 2=DPF_REGEN_HI, 3=NSC_DE_NOX_REGEN, 4=NSC_DE_SOX_REGEN, 5=SCR_HEATUP_STRATEGY

	//
	uint8_t cruiseControlDisabled=1;
	uint8_t ACC_Disabled=1;
	uint8_t ACC_engaged=0;
	uint8_t wheelPressedButtonID=0x10; //0x10= released, 0x20=strong speed decrease, 0x18=speed decrease, 0x00=strong speed increase, 0x08=speed increase, 0x90=RES, CC on/off=0x12
	uint8_t  lastPressedWheelButton=0xff; //default value, means no button pressed on the wheel
	uint32_t lastPressedWheelButtonTime=0;//stores the last time a wheel button was pressed, in msec from boot
	uint32_t lastPressedWheelButtonDuration=0x00; //default value
	uint32_t lastPressedSpeedUpWheelButtonDuration=0x00; //default value

	//ROUTE_MSG
	uint8_t function_route_msg_enabled=0; //default disabled . stored in flash
	uint8_t routeStdIdMsg=0xff;
	uint8_t routeOffset=0;
	uint32_t routeMsgId=0;
	CAN_TxHeaderTypeDef routeMsgHeader={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DAF1BA, .DLC=8};
	uint8_t routeMsgData[8]= {0x07, 0x62, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

	//REMOTE_START_ENABLED
	uint8_t function_remote_start_Enabled=0; //default disabled . stored in flash
	uint8_t engineRemoteStartRequest=0;
	uint32_t doorOpenTime=0;
	CAN_TxHeaderTypeDef REMOTE_START_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x1EF, .DLC=8};
	uint8_t REMOTE_START_msg_data[8]={0x42,0x04,0x96,0,};
	uint8_t RF_fob_number=0;
	uint8_t pressStartButton=0;
	CAN_TxHeaderTypeDef BODY4_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8};


	//READ_FAULTS_ENABLED
	uint8_t function_read_faults_enabled=1;

	//_4WD_DISABLER_ENABLED
	uint8_t function_4wd_disabler_enabled=1; //default enabled . stored in flash
	CAN_TxHeaderTypeDef driveTrainControlModuleResetMsgHeader[4]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=7},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA1AF1, .DLC=3}};
	uint8_t driveTrainControlModuleResetMsgData[4][8]= {{0x02, 0x11, 0x01,},{0x06, 0x2F, 0x2A, 0xAA, 0x03, 0x00, 0x00,},{0x02, 0x3E, 0x80,},{0x02, 0x10, 0x03,}}; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(set front torque to 0), reset ECU
	uint32_t last_sent_drive_train_msg_time=0;

	//IPC_MY23_IS_INSTALLED
	uint8_t function_ipc_my23_is_installed=1;

	//REGENERATION_ALERT_ENABLED
	uint8_t function_regeneration_alert_enabled=0;
	uint8_t regenerationInProgress=0;
	uint8_t STATUS_ECM_msg_data[8];
	CAN_TxHeaderTypeDef STATUS_ECM_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5AE, .DLC=8};

	//SHIFT_INDICATOR_ENABLED
	uint8_t function_shift_indicator_enabled=0; //saved in flash.
	CAN_TxHeaderTypeDef shift_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x2ED, .DLC=8};
	uint8_t shift_msg_data[8];

	#if defined(SHIFT_THRESHOLD)
		uint16_t shift_threshold=SHIFT_THRESHOLD;
	#else
		uint16_t shift_threshold=1500;
	#endif

	//LED_STRIP_CONTROLLER_ENABLED
	uint8_t function_led_strip_controller_enabled=0; //defines is not enough, by default leds are disabled.stored in ram
	float scaledVolume;
	uint8_t scaledColorSet;
	uint32_t timeSinceLastReceivedAcceleratorMessage=0;
	uint8_t ledsStripIsOn=0; //indicates if leds strip is on

	//DYNO_MODE_MASTER
	uint8_t function_dyno_mode_master_enabled=1; //stored in flash

	//ESC_TC_CUSTOMIZATOR_MASTER)
	uint8_t function_esc_tc_customizator_enabled=0; //stored in flash

	//FRONT_BRAKE_FORCER_MASTER
	uint8_t function_front_brake_forcer_master=1; //stored in flash
#endif

#if defined(C2baccable)
	//ESC_TC_CUSTOMIZATOR_ENABLED
	uint8_t currentDNAmode;
	uint8_t DNA_msg_data[8];
	CAN_TxHeaderTypeDef DNA_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x384, .DLC=8};
	uint8_t ESCandTCinversion=0; //0=do't perform anything, 1=disable ESC and TSC in D,N,A modes and enable ESC and TSC in race mode//---// used when ESC_TC_CUSTOMIZATOR_ENABLED is defined (also last 2 declarations)
	uint32_t LANEbuttonPressLastTimeSeen=0; //stores time (in milliseconds from power on) when LANE button (left stalk button) press was read last time
	uint8_t LANEbuttonPressCount=0; //stores number of times this message field was received

	//DYNO_MODE
	uint8_t DynoModeEnabled=0;
	uint8_t DynoStateMachine=0xff; //State machine for dyno messages sequence. frm 00 to 03 = dyno message sequence is beeing transmitted. FF= inactive
	uint16_t testerMsgSent=0;
	uint8_t DYNO_msg_data[5][6]={{0x02,0x10,0x03,},{0x03,0x22,0x30,0x02,},{0x05,0x2E,0x30,0x02,0x00,0x01},{0x05,0x2E,0x30,0x02,0xFF,0x01}}; //index0=diagnostic session, index1=read status, index2=disable dyno, index3=enable dyno
	CAN_TxHeaderTypeDef DYNO_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=6};

	uint32_t DynoStateMachineLastUpdateTime=0; //stores time (in milliseconds from power on) when Park Assist button press was read last time
	uint8_t ParkAssistButtonPressCount=0; //stores number of times this message field was received

	//FRONT_BRAKE_FORCER
	uint32_t last_sent_rear_brake_msg_time=0;

	CAN_TxHeaderTypeDef rearBrakeMsgHeader[4]={{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=5},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=8},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3},{.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA28F1, .DLC=3}};
	uint8_t rearBrakeMsgData[4][8]= {{0x04, 0x2F, 0x5A, 0xBD, 0x00,},{0x07, 0x2F, 0x5A, 0xBD, 0x03, 0x27, 0x10, 0x03},{0x02, 0x3E, 0x80,},{0x02, 0x10, 0x40,}}; //from last to first we have: diag session, tester present, IO Control - Short Term Adjustment(disable front brakes) (periodic)

#endif

#if defined(BHbaccable)
	uint32_t lastSentTelematic_display_info_msg_Time=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	uint8_t telematic_display_info_field_totalFrameNumber=(DASHBOARD_MESSAGE_MAX_LENGTH / 3) - 1; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5) //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_frameNumber=0; //current frame //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t telematic_display_info_field_infoCode=0x09; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t paramsStringCharIndex=0; // next char to send index - Used with SHOW_PARAMS_ON_DASHBOARD define functionality.
	CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8}; //used when SHOW_PARAMS_ON_DASHBOARD is defined
	uint8_t telematic_display_info_msg_data[8]; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality
	uint8_t requestToSendOneFrame=0; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard

	//Message to generate sound indication (chime)
	uint8_t CHIME_msg_data[8];
	CAN_TxHeaderTypeDef CHIME_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x5AC, .DLC=8};
	uint8_t requestToPlayChime=0;

#endif

//CLEAR_FAULTS_ENABLED
uint8_t function_clear_faults_enabled=1; //default enabled. saved on flash
uint8_t clearFaultsRequest=0; //if enabled, sends messages to clear faults
uint32_t last_sent_clear_faults_msg=0;
uint8_t clearFaults_msg_data[5]={0x04,0x14,0xff,0xff,0xff}; //message to clear DTC
CAN_TxHeaderTypeDef clearFaults_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA00F1, .DLC=5};

//
uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]={' ',}; //used if SHOW_PARAMS_ON_DASHBOARD or SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is declared - it contains string to print on dashboard

float currentSpeed_km_h=0; //current vehicle speed

uint32_t weCanSendAMessageReply=0; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
uint32_t debugTimer0;

UART_HandleTypeDef huart2; // this is the serial line between baccables -- used with SHOW_PARAMS_ON_DASHBOARD and SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE and low consumption define functionalities

uint32_t currentRpmSpeed=0;	//used by C1baccable
uint8_t currentGear=0; 		//used by C1baccable

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0,};  //msg data

uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

uint8_t _4wd_disabled=0; //if =4 disables 4wd
uint8_t front_brake_forced=0; //if=5 disables Front brakes
uint8_t commandsMenuEnabled=1; //if 0 disables the up-down buttons to change menu position

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
	#if defined(C1baccable)
		lowConsume_init();
		immobilizerEnabled = (uint8_t)readFromFlash(1);  //parameter1 stored in ram, so that we can get it. By default Immo is enabled
		if(immobilizerEnabled) executeDashboardBlinks=2; //shows the user that the immobilizer is active (or not)
		function_smart_disable_start_stop_enabled=(uint8_t)readFromFlash(2);  //parameter2 stored in ram, so that we can get it. By default S&S is enabled
		function_led_strip_controller_enabled=(uint8_t)readFromFlash(3); //By default led is disabled
		function_shift_indicator_enabled=(uint8_t)readFromFlash(4); //By default it is disabled
		shift_threshold= (uint16_t)readFromFlash(5);
		function_ipc_my23_is_installed=	(uint8_t)readFromFlash(6);
		function_route_msg_enabled=(uint8_t)readFromFlash(7);
		function_dyno_mode_master_enabled=(uint8_t)readFromFlash(8);
		function_acc_virtual_pad_enabled=(uint8_t)readFromFlash(9);
		function_front_brake_forcer_master=(uint8_t)readFromFlash(10);
		function_4wd_disabler_enabled=(uint8_t)readFromFlash(11);
		function_remote_start_Enabled=(uint8_t)readFromFlash(12);
		function_clear_faults_enabled=(uint8_t)readFromFlash(13);
		function_esc_tc_customizator_enabled=(uint8_t)readFromFlash(14);
		function_read_faults_enabled=(uint8_t)readFromFlash(15);
		function_is_diesel_enabled=(uint8_t)readFromFlash(16);
		function_regeneration_alert_enabled=(uint8_t)readFromFlash(17);
	#endif




	#if (defined(C1baccable) || defined(C2baccable) || defined(BHbaccable) || defined(ACT_AS_CANABLE))
		uart_init();
	#endif

	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (defined(C1baccable) || defined(C2baccable) )  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(BHbaccable))
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

	#if defined(DEBUG)
		uint8_t firstExecution=0;
	#endif

	while (1){

		debugTimer0=HAL_GetTick();
		onboardLed_process();
		can_process();
		processUART();
		//if(function_led_strip_controller_enabled>1){
		//	onboardLed_red_on();
		//}
		#if defined(DEBUG)
			if(debugTimer0>10000){
				//baccableDashboardMenuVisible=1;
			}
			if(debugTimer0>10000 && firstExecution==0){
				//clearFaultsRequest=255;
				//firstExecution=1;
				baccableDashboardMenuVisible=1;
				dashboard_menu_indent_level=1;
				main_dashboardPageIndex=1;
				dashboardPageIndex=20;
			}
		#endif
		#if defined(ACT_AS_CANABLE)
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

		#if defined(C1baccable)
			lowConsume_process();

			if(function_led_strip_controller_enabled==1){
				//don't act as canable. One USB port pin is used to control leds.
				vuMeterInit(); //initialize leds strip controller - this is called many times to divide the operations on more loops
				if(ledsStripIsOn){ //if the strip is on,
					if(timeSinceLastReceivedAcceleratorMessage+10000<HAL_GetTick() ){ //if no can interesting message for 10 seconds,  shutdown the leds to save energy
						shutdownLedsStrip();
						ledsStripIsOn=0; //entriamo solo una volta
					}
				}
			}

			if(function_4wd_disabler_enabled==1){
				if(_4wd_disabled>0){
					uint8_t tempDeltaTime=0;
					if(_4wd_disabled>1) tempDeltaTime=70;
					if(HAL_GetTick()-last_sent_drive_train_msg_time>30+tempDeltaTime){ //enter here once each 100msec, then each 30msec
						last_sent_drive_train_msg_time=HAL_GetTick();
						onboardLed_blue_on();
						can_tx(&driveTrainControlModuleResetMsgHeader[_4wd_disabled-1], driveTrainControlModuleResetMsgData[_4wd_disabled-1]); //send drive train control module message
						if(_4wd_disabled>1) _4wd_disabled--;
					}
				}
			}

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

			if(function_smart_disable_start_stop_enabled){
				if(startAndStopEnabled){
					if(HAL_GetTick()>10000){ //first 10 seconds don't do anything to avoid to disturb other startup functions or immobilizer
						if(currentRpmSpeed>400){ //if motor is on

							if(startAndstopCarStatus==0){//if start & stop was found disabled in car, we don't need to do anything. Avoid to enter here; We enter here in example if board is switched when the car is running and S&S was still manually disabled by the pilot
								startAndStopEnabled=0;
								requestToDisableStartAndStop=0;
							}else{
								if(lastTimeStartAndstopDisablerButtonPressed==0){ //first time we arrive here, go inside
									requestToDisableStartAndStop=1;

									//#if defined(DISABLE_START_STOP)
									//	// We will now short gpio to ground in order to disable start&stop car functionality. This will simulate car start&stop button press
									//	HAL_GPIO_WritePin(START_STOP_DISABLER, 0); // I use swclk pin, pin37, PA14
									//	lastTimeStartAndstopDisablerButtonPressed=HAL_GetTick();
									//	onboardLed_blue_on();
									//#endif
								}
								//#if defined(DISABLE_START_STOP)
								//	if(HAL_GetTick()-lastTimeStartAndstopDisablerButtonPressed>2000){ //if pressed since 500msec
								//		HAL_GPIO_WritePin(START_STOP_DISABLER, 1); //return to 1
								//		startAndStopEnabled=0;
								//		onboardLed_blue_on();
								//	}
								//#endif
							}
						}
					}
				}
			}

			if(shutdownDashboardMenuRequestTime>0){
				if(HAL_GetTick()-shutdownDashboardMenuRequestTime>39000){
					baccableDashboardMenuVisible=0; //stop sending params request when motor is off
					clearDashboardBaccableMenu(); //ripulisci la stringa
					baccabledashboardMenuWasVisible=1; //allows the menu to automatically turn on when motor rotates
					shutdownDashboardMenuRequestTime=0; //avoid to return here
				}
			}
			//send a parameter request each xx msec if dashboard menu shall be visible
			//baccableDashboardMenuVisible=1; //force menu always on, just for debug
			if((HAL_GetTick()-last_sent_uds_parameter_request_Time>500) && baccableDashboardMenuVisible ){
				last_sent_uds_parameter_request_Time=HAL_GetTick();

				switch(dashboard_menu_indent_level){
					case 0: //main menu
						sendMainDashboardPageToSlaveBaccable();
						break;
					case 1:
						if(main_dashboardPageIndex==1){ //we are in show params submenu
							clearFaultsRequest=0; //ensure we don't perform more tasks simoultaneously
							if(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId>0xFF){ //if req id is greather than 0xFF it is a standard UDS request.
								//request current parameter to ECU
								uds_parameter_request_msg_header.ExtId=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId;


								memcpy(&uds_parameter_request_msg_data[0],&uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqData,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqLen );
								uds_parameter_request_msg_header.DLC=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqLen;
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

						if(main_dashboardPageIndex==9){
							sendSetupDashboardPageToSlaveBaccable();
						}
						break;
					default:
						break; //unexpected
				}


			}
		#endif

		#if defined(C2baccable)
			if(DynoStateMachine!=0xff){ //if state machine in progress
				if(DynoStateMachineLastUpdateTime+4000< HAL_GetTick()){ //if older than 4 sec
					DynoStateMachine=0xff; //timeout. stop any sequence
				}
			}

			if(front_brake_forced==255){ //request to disable Front brake
				front_brake_forced=0;
				//just reply to C1 baccable
				uint8_t tmpArr[2]={C1BusID,C2cmdNormalFrontBrake};
				addToUARTSendQueue(tmpArr, 2);
				can_tx(&rearBrakeMsgHeader[0], rearBrakeMsgData[0]); //send message to return control to ECU
				//can_tx(&rearBrakeMsgHeader[0], rearBrakeMsgData[0]); //send message to return control to ECU


			}

			if(front_brake_forced==5){
				front_brake_forced=4;
				//send reply via serial line to C1 to inform that front brake is going to be forced
				uint8_t tmpArr[2]={C1BusID,C2cmdForceFrontBrake};
				addToUARTSendQueue(tmpArr, 2);
			}

			if(front_brake_forced>0){ //force front brake
				//we shall send msg sequence
				if(HAL_GetTick()-last_sent_rear_brake_msg_time>500){ //enter here once each 500msec
					last_sent_rear_brake_msg_time=HAL_GetTick();
					onboardLed_blue_on();
					can_tx(&rearBrakeMsgHeader[front_brake_forced-1], rearBrakeMsgData[front_brake_forced-1]); //send message to force front brakes

					switch(front_brake_forced){
						case 4:
						case 3:
							front_brake_forced--;
							break;
						case 2:
							front_brake_forced++;
							break;
						default:
							break;
					}

				}
			}

		#endif

#if defined(BHbaccable) //this is the baccable slave
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
			/*
			if (paramsStringCharIndex==3){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						indiceTmp++;
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)(indiceTmp & 0x00FF); //
						indiceTmp++;
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)(indiceTmp & 0x00FF); //
						indiceTmp++;
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)(indiceTmp & 0x00FF); //

					}
				}
			}

			if (paramsStringCharIndex==6){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						indiceTmp++;
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)(indiceTmp & 0x00FF); //
						indiceTmp++;
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)(indiceTmp & 0x00FF); //
						indiceTmp++;
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)(indiceTmp & 0x00FF); //

					}
				}
			}

			if (paramsStringCharIndex==6){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)((indiceTmp--) & 0x0F); //
					}
				}
			}

			if (paramsStringCharIndex==9){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)((indiceTmp--) & 0x0F); //
					}
				}
			}

			if (paramsStringCharIndex==12){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)((indiceTmp--) & 0x0F); //
					}
				}
			}

			if (paramsStringCharIndex==15){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)((indiceTmp--) & 0x0F); //
					}
				}
			}

			if (paramsStringCharIndex==18){
				if(dashboardPageStringArray[0]=='['){
					if(dashboardPageStringArray[1]=='X'){
						//replace char to use flagged checkbox
						telematic_display_info_msg_data[2]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[3]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[4]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[5]=(uint8_t)((indiceTmp--) & 0x0F); //
						telematic_display_info_msg_data[6]=(uint8_t)(indiceTmp>>8);
						telematic_display_info_msg_data[7]=(uint8_t)((indiceTmp--) & 0x0F); //
					}
				}
			}

*/
			can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet

			telematic_display_info_field_frameNumber++; //prepare for next frame to send
			if( paramsStringCharIndex>=DASHBOARD_MESSAGE_MAX_LENGTH) { //if we sent the entire string
				paramsStringCharIndex=0; //prepare to send first char of the string
				telematic_display_info_field_frameNumber=0; //prepare to send first frame
				requestToSendOneFrame -= 1;
				onboardLed_blue_on();
			}
		}
	}
#endif

#if (defined(C1baccable) || defined(C2baccable) || defined(BHbaccable))
	if(clearFaultsRequest>0){
		//clear faults if requested
		if(HAL_GetTick()-last_sent_clear_faults_msg>25){
			last_sent_clear_faults_msg= HAL_GetTick();

			#if defined(C1baccable)
				if(clearFaultsRequest==255){
					//ask to Baccable on C2 and bH bus, to reset faults //to be done
					uint8_t tmpArr[1]={AllResetFaults};
					addToUARTSendQueue(tmpArr, 1);
				}
			#endif
			//send a reset request
			clearFaults_msg_header.ExtId= 0x18DA00F1 | ((uint32_t)clearFaultsRequest<<8);
			can_tx(&clearFaults_msg_header, clearFaults_msg_data); //transmit the request

			clearFaultsRequest--;
		}
	}
#endif

		// If CAN message receive is pending, process the message
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					onboardLed_blue_on();
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#endif

				#if defined(C1baccable)
					lastReceivedCanMsgTime=HAL_GetTick();
				#endif
				if (rx_msg_header.RTR == CAN_RTR_DATA){
					switch(rx_msg_header.IDE){
						case CAN_ID_EXT:
							#if defined(C1baccable)
								if(immobilizerEnabled){
									//if it is a message of connection to RFHUB, reset the connection periodically, but start the panic alarm only once
									if(floodTheBus==0){ //if we are not flooding the bus
										uint8_t responseOffset=rx_msg_data[0]>>4; //0=single frame , 1=first fragmented frame 2=fragmented frame, 3=frame ack
										if((rx_msg_header.ExtId & 0xFFFFFFF0)==0x18DAC7F0){ 		//if it is message from the thief
											if(responseOffset<2){ //we pass this if, in case of single frame and first fragmented frame
												switch(rx_msg_data[responseOffset+1]){
													case 0x10: //diagnostic session
													case 0x27: //security access
													case 0x29: //authentication
													case 0x3E: //tester presence
													//case 0x1A: //??
													case 0x2E: //write data by identifier
													case 0x3D: //write memory by address
														floodTheBus=1; //reset the RFHUB and start the alarm
														break;
													default:
														break;
												}
											}
										}else if((rx_msg_header.ExtId & 0xFFFFF0FF)==0x18DAF0C7) { 	//if it is a reply from rfhub
											//if(floodTheBusStartTime==0){ //this allows to read rfhub messages only if it was the first time
												if(responseOffset<2){ //we pass this if, in case of single frame and first fragmented frame
													switch(rx_msg_data[responseOffset+1]){
														case 0x50: //diagnostic session	//
														case 0x67: //security access
														case 0x69: //authentication
														case 0x7E: //tester presence 	//
														//case 0x1A: //??
														case 0x6E: //write data by identifier
														case 0x7D: //write memory by address
															floodTheBus=1; //reset the RFHUB and start the alarm
															break;
														default:
															break;
													}
												}
											//}
										}
										if(floodTheBus==1){ //if we engaged the immobilizer
											floodTheBusStartTime=HAL_GetTick(); //set initial time we started to flood the bus
											onboardLed_blue_on(); //light a led
										}
										//if ((rx_msg_header.ExtId==0x18DAC7F1) ||( (rx_msg_header.ExtId==0x18DAF1C7) && (floodTheBusStartTime==0)) ){  // If msg from thief or (reply from rfhub && it is the first time that it occurs)
										//	//thief connected to RFHUB: we shall reset the RFHUB and start the alarm
										//	//start to flood the bus with the rfhub disconnect message
										//	floodTheBus=1;
										//	floodTheBusStartTime=HAL_GetTick();
										//	onboardLed_blue_on();
										//}
									}
								} //end of immobilizer section

								if ((rx_msg_header.ExtId==uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyId) && baccableDashboardMenuVisible){ //if we received udf message with current selected parameter, let's aquire it
									if(dashboard_menu_indent_level==1 && main_dashboardPageIndex==1){ //if we are in show params menu
										onboardLed_blue_on();
										if (rx_msg_header.DLC>=4+uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyOffset+uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyLen){
											uint8_t numberOfBytesToRead=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyLen;
											// Limita il numero di byte a un massimo di 4 per evitare overflow
											if (numberOfBytesToRead > 4) {
												numberOfBytesToRead = 4;
											}
											uint32_t tmpVal=0; //take value of received parameter

											// Costruisce il valore a partire dai byte ricevuti
											for (size_t i = 0; i < numberOfBytesToRead; i++) {
												tmpVal |= ((uint32_t)rx_msg_data[4+uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyOffset+i]) << (8 * (numberOfBytesToRead - 1 - i));
											}

											tmpVal+=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyValOffset;
											float tmpVal2 =tmpVal * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale;
											tmpVal2 +=uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;

											sendDashboardPageToSlaveBaccable(tmpVal2);//send parameter
										}
									}
								}

								if(function_route_msg_enabled==1){
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

									if(baccableDashboardMenuVisible) routeStdIdMsg=0xff; //disables the route request, to avoid conflicts with show params functionality

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
								}
							#endif //end define

							#if defined(C2baccable)
								if (rx_msg_header.ExtId==0x18DAF128 && DynoStateMachine!=0xff ){ //if message from ABS ECU and Dyno state machine is in progress
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
							/*
							#if defined(LOW_CONSUME)
								if (rx_msg_header.ExtId==0x1E340000){ //if we received the "Wake" message from BCM
									if((rx_msg_data[0]>>7)==1){ // if the "Main wakeup status from BCM" field is active
										wakeUpAllProcessorsAndTransceivers();
									}else{ //else, we should go to sleep
										reduceConsumption();
									}
								}
							#endif
							*/
							break;
						case CAN_ID_STD: //if standard ID
							#if defined(C1baccable)
								if(function_route_msg_enabled==1){
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
								}
							#endif

							switch(rx_msg_header.StdId){ //messages in this switch is on C can bus, only when on different bus, the comments explicitly tells if it is on another can bus

								case 0x00000090:
									#if defined(BHbaccable) //the car is showing something (ie:radio name) on the dashboard
										//override the displaied string, by restarting the frame
										paramsStringCharIndex=0; //prepare to send first char of the string
										telematic_display_info_field_frameNumber=0; //prepare to send first frame
										if((requestToSendOneFrame==0) && (dashboardPageStringArray[0]!=' ')) requestToSendOneFrame++; //if message is not empty (we check only first char) and if required, increment messages sequence to send, in order to send at least one
										lastSentTelematic_display_info_msg_Time=0; //if required will immediately send a sequence
									#endif
									//on BH can bus, slow bus at 125kbps, this message contains:

									//total frame number is on byte 0 from bit 7 to 3
									//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
									//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
									//UTF text 1 is on byte 2 and byte 3
									//UTF text 2 is on byte 4 and byte 5
									//UTF text 3 is on byte 6 and byte 7
									break;
								case 0x000000FB:

									#if defined(C1baccable)
										if(rx_msg_header.DLC>=4){
											torque= ((rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111));
										}
									#endif
									//torque is on byte 2 from bit 6 to 0 and byte 3 from bit 7 to 4.
									break;

								case 0x000000FC: //message to dashboard containing rpm speed and not only
									#if (defined(C1baccable) || defined(C2baccable))
										if(rx_msg_header.DLC>=2){
											currentRpmSpeed=(rx_msg_data[0] *256 + (rx_msg_data[1] & ~0x3) )/4; //extract rpm speed
											//onboardLed_blue_on();
										}
									#endif
									#if defined(C2baccable)
										if(currentRpmSpeed< 400 ){
											if(ESCandTCinversion!=0) ESCandTCinversion=0;
											if(DynoModeEnabled!=0) DynoModeEnabled=0;
											if(front_brake_forced!=0) front_brake_forced=255;
										}
									#endif
									#if defined(C1baccable)
										if(currentRpmSpeed<400){
											startAndStopEnabled=1; //if motor off, re-enable start&stop logic
											requestToDisableStartAndStop=0; //re set request trigger

											if(_4wd_disabled>0){
												//disable 4dw function
												_4wd_disabled=0; //disable 4dw function
												dashboard_main_menu_array[main_dashboardPageIndex][4]=' '; //enabled
												dashboard_main_menu_array[main_dashboardPageIndex][5]='E';
												dashboard_main_menu_array[main_dashboardPageIndex][6]='n';
												commandsMenuEnabled=1;//enable menu commands
											}

											if(baccableDashboardMenuVisible==1){
												if(shutdownDashboardMenuRequestTime==0) shutdownDashboardMenuRequestTime=HAL_GetTick(); //save time. we will shut it off after one minute
											}
										}else{
											shutdownDashboardMenuRequestTime=0;
											if(baccabledashboardMenuWasVisible==1){
												baccableDashboardMenuVisible=1; //show menu
												baccabledashboardMenuWasVisible=0; //avoid to return here
											}
										}
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
								case 0x00000101:
									#if defined(C1baccable)
										if(rx_msg_header.DLC>=3){
											//get vehicle speed
											currentSpeed_km_h= (float)((((uint16_t)rx_msg_data[0] << 11) & 0b0001111111111111) | ((uint16_t)rx_msg_data[1] <<3) | ((uint16_t)rx_msg_data[2] >>5))/16;
										}
									#endif
									break;
								case 0x000001EF:
									#if defined(C1baccable)
										if(function_remote_start_Enabled==1){
											if(rx_msg_header.DLC==8){
												if(rx_msg_data[2]>>4==0x4){ //it is the message to open the car. THIS IS JUST FOR TEST!!!!!
												RF_fob_number=rx_msg_data[1] & 0x1E; //store fob id (the real id is obrainable by shifting by 1 bit to the right, but this value is better, in order to be used in the next message
												//within 2 seconds, send remote start
												doorOpenTime=HAL_GetTick();
												engineRemoteStartRequest=3;
												}

												if(engineRemoteStartRequest){
													if(doorOpenTime+2000<HAL_GetTick()){ //we have to start the engine
														memcpy(&REMOTE_START_msg_data, &rx_msg_data, 8);
														//REMOTE_START_msg_data[0]= (REMOTE_START_msg_data[0] & 0x0F ) | 0x80; //set custom key ignition status= custom key in ignition

														REMOTE_START_msg_data[1]=RF_fob_number;//update the fob number
														if(engineRemoteStartRequest==3){ //first message let's close the doors
															REMOTE_START_msg_data[2]=0x16;//update the function request (1= lock ports) and requestor (6=remote_start)
															doorOpenTime=HAL_GetTick()-1000; //refresh time, so that we will wait 2 more seconds

														}else{
															REMOTE_START_msg_data[2]=0x96;//update the function request (9= remote start) and requestor (6=remote_start)
															//REMOTE_START_msg_data[2]=0x92;//update the function request (9= remote start) and requestor (2=original keyless)
														}
														REMOTE_START_msg_data[6] ++; //update the counter
														if(REMOTE_START_msg_data[6]>0x0F) REMOTE_START_msg_data[6]=0; //check the counter

														REMOTE_START_msg_data[7] = calculateCRC(REMOTE_START_msg_data,REMOTE_START_msg_header.DLC); //update checksum
														can_tx(&REMOTE_START_msg_header, REMOTE_START_msg_data); //send msg

														//transmit one more message
														//if(engineRemoteStartRequest==2){
														//	REMOTE_START_msg_data[6] ++; //update the counter
														//	if(REMOTE_START_msg_data[6]>0x0F) REMOTE_START_msg_data[6]=0; //check the counter
														//	REMOTE_START_msg_data[7] = calculateCRC(REMOTE_START_msg_data,REMOTE_START_msg_header.DLC); //update checksum
														//	can_tx(&REMOTE_START_msg_header, REMOTE_START_msg_data); //send msg
														//	doorOpenTime=HAL_GetTick()+5000; //set the trigger in order to enter next 10 seconds
														//}

														//if(engineRemoteStartRequest==1){

														//	pressStartButton=1;
														//}
														onboardLed_blue_on();
														if (engineRemoteStartRequest>0) engineRemoteStartRequest--; //avoid to return here after the required messages were sent
													}
												}
											}
										}
									#endif
									break;
								case 0x000001F0:
									//clutch interlock is on byte 0 bit 7
									//clutch upstop is on byte0 bit 6
									//actual pedal position is on byte0 from bit 4 to 0 and byte 1 from bit7 to 5
									//analog cluch is on byte 1 from bit 4 to 0 and byte 2 from bit 7 to 5.

									break;
								case 0x0000001F7:
									#if defined(C1baccable)
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
									#if defined(C1baccable)
										if(rx_msg_header.DLC>=3){
											//fill a variable with start&stop Status (byte2 bit 3 and 2 = 1 means S&S disabled)
											if(((rx_msg_data[2]>>2) & 0x03) ==0x01){
												startAndstopCarStatus=0; //S&S disabled in car

											}else{
												startAndstopCarStatus=1;//S&S enabled in car (default in giulias)
											}
										}
									#endif
									break;
								case 0x000002ED: //message to dashboard containing shift indicator
									#if defined(C1baccable)
										if(function_shift_indicator_enabled==1){
											if(rx_msg_header.DLC==8){
												if (currentRpmSpeed>shift_threshold-1 ){ //if the rpm speed is above SHIFT_THRESHOLD rpm, then the packet need to be modified, therefore,
													//copy 8 bytes from msgdata rx (rx_msg_data) to msg data tx (shift_msg_data)
													memcpy(&shift_msg_data, &rx_msg_data, 8);

													if(function_ipc_my23_is_installed){ //on IPC my23 it is a little bit different
														//change byte1, bit 6 and 5(lsb) to 10 (binary) meaning "Upshift suggestion"
														shift_msg_data[1] = (shift_msg_data[1] & ~0x60) | (0x40 & 0x60);
													}

													if(currentRpmSpeed>(shift_threshold-1) &&  currentRpmSpeed<(shift_threshold+500)){ //set lamp depending on rpm speed
														//change byte6, bit 1 and 0(lsb) to 01 (binary) meaning "gear shift urgency level 1"
														shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x1 & 0x3);
													}
													if(currentRpmSpeed>(shift_threshold+500-1) &&  currentRpmSpeed<(shift_threshold+1000)){ //set lamp depending on rpm speed
														//change byte6, bit 1 and 0(lsb) of the message to 10 (binary) meaning "gear shift urgency level 2"
														shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x2 & 0x3);
													}else if(currentRpmSpeed>(shift_threshold+1000-1)){ //set lamp depending on rpm speed
														//change byte6, bit 1 and 0(lsb) of the message to 11 (binary) meaning "gear shift urgency level 3"
														shift_msg_data[6] = (shift_msg_data[6] & ~0x3) | (0x3 & 0x3);
													}
													can_tx(&shift_msg_header, shift_msg_data); //transmit the modified packet
													onboardLed_blue_on();
												}

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
									#if defined(C1baccable)
										currentGear=rx_msg_data[0] & ~0xF;

										if(function_led_strip_controller_enabled==1){
											scaledColorSet=scaleColorSet(currentGear); //prima di tutto azzeriamo i primi 4 bit meno significativi, poi scala il dato con la funzione scaleColorSet, per prepararlo per l'invio alla classe vumeter
											vuMeterUpdate(scaledVolume,scaledColorSet);
										}

										if(function_regeneration_alert_enabled){
											if((rx_msg_data[1] >>7)==1 && regenerationInProgress==0){ //if regeneration has just begun,
												uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
												addToUARTSendQueue(tmpArr3, 1);
											}
										}
										regenerationInProgress=rx_msg_data[1] >>7; //DPF Regeneration mode is on byte 1 bit 7.

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

									//function ACC Virtual Pad
									#if defined(C1baccable)
										if(function_acc_virtual_pad_enabled==1){
											switch (rx_msg_data[0]){
												case 0x12: //CC on
													newWheelPressedButtonID=0x12;
													memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
													ACC_msg_data[0] = 0x11; //ACC On
													ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
													ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
													can_tx(&ACC_msg_header, ACC_msg_data); //send msg
													onboardLed_blue_on();
													break;
												case 0x90: //RES pressed
													if (newWheelPressedButtonID==0x10 && ACC_engaged) ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=1; //if begin to press button RES and ACC is Engaged, set ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED
													newWheelPressedButtonID=0x90; //store the new RES button status (pressed)
													if (ACC_engaged && ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED){
														//simulate the distance button press
														memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
														ACC_msg_data[0] = 0x50; //ACC distance change
														ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
														ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
														can_tx(&ACC_msg_header, ACC_msg_data); //send msg
														onboardLed_blue_on();
													}
													break;
												case 0x10: //button released
													newWheelPressedButtonID=0x10; //button released (I use another variable to distinguish from the one used in show params function
													ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=0;
													break;
												default:
											}
										}

									//This is used if the SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE is defined
										if(cruiseControlDisabled && ACC_Disabled){ //if we are allowed to press buttons, use them in baccable menu
											switch(rx_msg_data[0]){
												case 0x18://if cruise control speed reduction button was pressed, user wants to see next page
													if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
														wheelPressedButtonID=0x18; //avoid to return here
														if(commandsMenuEnabled){
															switch(dashboard_menu_indent_level){
																case 0: //main menu
																	main_dashboardPageIndex+= 1; //set next page

																	if(function_read_faults_enabled==0){
																		if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
																	}


																	if(function_clear_faults_enabled==0){
																		if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
																	}

																	if(function_dyno_mode_master_enabled==0){
																		if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
																	}

																	if(function_esc_tc_customizator_enabled==0){
																		if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
																	}
																	if(function_front_brake_forcer_master==0){
																		if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
																	}

																	if(function_4wd_disabler_enabled==0){
																		if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
																	}

																	if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
																	//onboardLed_blue_on();
																	sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
																	break;
																case 1:
																	if(main_dashboardPageIndex==1){ //we are in show params submenu
																		dashboardPageIndex += 1; //set next page
																		if(function_is_diesel_enabled==1){
																			if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
																		}else{
																			if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
																		}
																			//onboardLed_blue_on();
																		sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
																	}
																	if(main_dashboardPageIndex==9){ //we are in setup menu
																		setup_dashboardPageIndex+=1;//set next page
																		if(setup_dashboardPageIndex==2) setup_dashboardPageIndex++; //future growth

																		if(setup_dashboardPageIndex==8) setup_dashboardPageIndex++; //future growth

																		if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu

																		//onboardLed_blue_on();
																		sendSetupDashboardPageToSlaveBaccable(); //send
																	}
																	break;
																default:
																	break; //unexpected
															}
														}
													}
													break;
												case 0x20://if cruise control speed strong reduction button was pressed, user wants to jump 10 pages forward
														if(wheelPressedButtonID==0x18 && baccableDashboardMenuVisible){ //if button released, use pressed button
															wheelPressedButtonID=0x20; //avoid to return here
															if(commandsMenuEnabled){
																switch(dashboard_menu_indent_level){
																	case 0: //main menu
																		main_dashboardPageIndex+= 1; //set next page
																		if(function_read_faults_enabled==0){
																			if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
																		}
																		if(function_clear_faults_enabled==0){
																			if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
																		}

																		if(function_dyno_mode_master_enabled==0){
																			if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
																		}
																		if(function_esc_tc_customizator_enabled==0){
																			if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
																		}
																		if(function_front_brake_forcer_master==0){
																			if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
																		}

																		if(function_4wd_disabler_enabled==0){
																			if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
																		}

																		if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
																		//onboardLed_blue_on();
																		sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
																		break;
																	case 1:
																		if(main_dashboardPageIndex==1){ //we are in show params submenu
																			dashboardPageIndex += 10; //set 9 pages forward (+1 in gentle command)
																			if(function_is_diesel_enabled==1){
																				if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
																			}else{
																				if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
																			}
																				//onboardLed_blue_on();
																			sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
																		}
																		if(main_dashboardPageIndex==9){ //we are in setup menu
																			setup_dashboardPageIndex+=10;//set next page
																			if(setup_dashboardPageIndex==2) setup_dashboardPageIndex++; //future growth
																			if(setup_dashboardPageIndex==8) setup_dashboardPageIndex++; //future growth
																			if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu
																			//onboardLed_blue_on();
																			sendSetupDashboardPageToSlaveBaccable(); //send
																		}
																		break;
																	default:
																		break; //unexpected
																}
															}
														}

														break;
												case 0x08: //if cruise control speed increase button was pressed, user wants to see previous page
													if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
														wheelPressedButtonID=0x08; //avoid to enter again here
														if(commandsMenuEnabled){
															switch(dashboard_menu_indent_level){
																case 0: //main menu
																	main_dashboardPageIndex-= 1; //set next page

																	if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu
																	if(function_4wd_disabler_enabled==0){
																		if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
																		}

																	if(function_front_brake_forcer_master==0){
																		if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
																	}
																	if(function_esc_tc_customizator_enabled==0){
																		if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
																	}
																	if(function_dyno_mode_master_enabled==0){
																		if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
																	}

																	if(function_clear_faults_enabled==0){
																		if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
																	}
																	if(function_read_faults_enabled==0){
																		if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
																	}
																	//onboardLed_blue_on();
																	sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
																	break;
																case 1:
																	if(main_dashboardPageIndex==1){ //we are in show params submenu
																		dashboardPageIndex -= 1; //set previous page
																		if(function_is_diesel_enabled==1){
																			if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=total_pages_in_dashboard_menu_diesel-1; // make a rotative menu
																		}else{
																			if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=total_pages_in_dashboard_menu_gasoline-1; // make a rotative menu
																		}
																		//onboardLed_blue_on();
																		sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
																	}
																	if(main_dashboardPageIndex==9){ //we are in setup menu
																		setup_dashboardPageIndex-=1;//set next page
																		if(setup_dashboardPageIndex==8) setup_dashboardPageIndex--; //future growth
																		if(setup_dashboardPageIndex==2) setup_dashboardPageIndex--; //future growth

																		if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=total_pages_in_setup_dashboard_menu-1; // make a rotative menu
																		//onboardLed_blue_on();
																		sendSetupDashboardPageToSlaveBaccable(); //send
																	}
																	break;
																default:
																	break; //unexpected
															}
														}
													}
													break;
												case 0x00: //if cruise control speed strong increase button was pressed, user wants to jump 10 pages before
														if(wheelPressedButtonID==0x08 && baccableDashboardMenuVisible){
															wheelPressedButtonID=0x00; //avoid to return here
															if(commandsMenuEnabled){
																switch(dashboard_menu_indent_level){
																	case 0: //main menu
																		main_dashboardPageIndex-= 1; //set next page
																		if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu
																		if(function_4wd_disabler_enabled==0){
																			if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
																		}
																		if(function_front_brake_forcer_master==0){
																			if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
																		}
																		if(function_esc_tc_customizator_enabled==0){
																			if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
																		}
																		if(function_dyno_mode_master_enabled==0){
																			if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
																		}

																		if(main_dashboardPageIndex==4) main_dashboardPageIndex--;

																		if(function_clear_faults_enabled==0){
																			if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
																		}
																		if(function_read_faults_enabled==0){
																			if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
																		}
																		//onboardLed_blue_on();
																		sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
																		break;
																	case 1:
																		if(main_dashboardPageIndex==1){ //we are in show params submenu
																			dashboardPageIndex -= 10; //set 10 pages backward
																			if(function_is_diesel_enabled==1){
																				if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // stay at zero.
																			}else{
																				if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // stay at zero.
																			}
																			//onboardLed_blue_on();
																			sendDashboardPageToSlaveBaccable(-3400000000000000000); //send dashboard page via usb
																		}
																		if(main_dashboardPageIndex==9){ //we are in setup menu
																			setup_dashboardPageIndex-=10;//set prev page
																			if(setup_dashboardPageIndex==8) setup_dashboardPageIndex--; //future growth
																			if(setup_dashboardPageIndex==2) setup_dashboardPageIndex--; //future growth
																			if(setup_dashboardPageIndex>=total_pages_in_setup_dashboard_menu)  setup_dashboardPageIndex=0; // make a rotative menu
																			//onboardLed_blue_on();
																			sendSetupDashboardPageToSlaveBaccable(); //send
																		}
																		break;
																	default:
																		break; //unexpected
																}
															}
														}
														break;
												case 0x10: // button released
													if(wheelPressedButtonID==0x89 && baccableDashboardMenuVisible==1){ //we pressed RES for at least one instant, then we released before 2 seconds, therefore we want to enter inside dashboard menu (will work only if menu is visible)

														if(dashboard_menu_indent_level==0){
															uint8_t printStopTheCar=0; //if enabled prints a message to screen for half second
															switch(main_dashboardPageIndex){
																case 1: //show params
																	dashboard_menu_indent_level++;
																	sendDashboardPageToSlaveBaccable(-3400000000000000000);
																	break;
																case 2: //read faults
																	//To Be Done
																	break;
																case 3: //clear faults
																	clearFaultsRequest=255;
																	break;
																case 4: //Immo
																	// nothing to do
																	break;
																case 5: // toggle dyno status
																	//send request thu serial line
																	if(currentSpeed_km_h==0){
																		uint8_t tmpArr1[2]={C2BusID,C2cmdtoggleDyno};
																		addToUARTSendQueue(tmpArr1, 2);
																	}else{
																		printStopTheCar=1;//print message "stop the car"
																	}
																	break;
																case 6: //toggle ESC/TC
																	//send request thu serial line
																	uint8_t tmpArr2[2]={C2BusID,C2cmdtoggleEscTc};
																	addToUARTSendQueue(tmpArr2, 2);
																	break;
																case 7: //toggle front brake

																	if(front_brake_forced>0){ //toggle front brake
																		//send serial message to C2 baccable, to set front brakes to normal
																		uint8_t tmpArr3[2]={C2BusID,C2cmdNormalFrontBrake};
																		addToUARTSendQueue(tmpArr3, 2);
																	}else{
																		if(currentSpeed_km_h==0){
																			//send serial message to C2 baccable, to force front brakes
																			uint8_t tmpArr4[2]={C2BusID,C2cmdForceFrontBrake};
																			addToUARTSendQueue(tmpArr4, 2);
																		}else{
																			printStopTheCar=1;//print message "stop the car"
																		}
																	}


																	break;
																case 8: //toggle 4wd
																	if(_4wd_disabled>0){ //toggle 4dw disable
																		_4wd_disabled=0;
																		//update text
																		dashboard_main_menu_array[main_dashboardPageIndex][4]=' '; //enabled
																		dashboard_main_menu_array[main_dashboardPageIndex][5]='E';
																		dashboard_main_menu_array[main_dashboardPageIndex][6]='n';
																		commandsMenuEnabled=1;//enable menu commands
																	}else{
																		if(currentSpeed_km_h==0){
																			_4wd_disabled=4;
																			//update text
																			dashboard_main_menu_array[main_dashboardPageIndex][4]='D'; //disabled
																			dashboard_main_menu_array[main_dashboardPageIndex][5]='i';
																			dashboard_main_menu_array[main_dashboardPageIndex][6]='s';
																			commandsMenuEnabled=0;//disable menu commands
																		}else{
																			printStopTheCar=1;//print message "stop the car"
																		}
																	}
																	break;
																case 9: //setup menu
																	dashboard_menu_indent_level++;

																	break;
																default:
																	break;
															}

															if(printStopTheCar==1){
																uint8_t stopTheCarMsg[13]={BhBusIDparamString,'S','T','O','P',' ','T','H','E',' ','C','A','R'};
																addToUARTSendQueue(stopTheCarMsg, 13);//print message "stop the car"
															}

														}else{ //indent level >0
															if(main_dashboardPageIndex==9){ //setup menu
																//uint8_t tmpArr[19];

																switch(setup_dashboardPageIndex){
																	case 0: //{'S','A','V','E','&','E','X','I','T',},
																		//if some change occurred
																		if(	((uint16_t)function_smart_disable_start_stop_enabled	!= readFromFlash(2)) 	|| //S&S enable status
																			((uint16_t)function_led_strip_controller_enabled		!= readFromFlash(3)) 	|| //LED_STRIP_CONTROLLER_ENABLED
																			((uint16_t)function_shift_indicator_enabled				!= readFromFlash(4)) 	|| //SHIFT_INDICATOR_ENABLED
																			((uint16_t)shift_threshold								!= readFromFlash(5))	|| //SHIFT_THRESHOLD
																			((uint16_t)function_ipc_my23_is_installed				!= readFromFlash(6))	|| //IPC_MY23_IS_INSTALLED
																			((uint16_t)function_route_msg_enabled					!= readFromFlash(7))	|| //ROUTE_MSG
																			((uint16_t)function_dyno_mode_master_enabled			!= readFromFlash(8))	|| //DYNO_MODE_MASTER
																			((uint16_t)function_acc_virtual_pad_enabled				!= readFromFlash(9))	|| //ACC_VIRTUAL_PAD
																			((uint16_t)function_front_brake_forcer_master			!= readFromFlash(10))	|| //FRONT_BRAKE_FORCER_MASTER
																			((uint16_t)function_4wd_disabler_enabled				!= readFromFlash(11))	|| //_4WD_DISABLER
																			((uint16_t)function_remote_start_Enabled				!= readFromFlash(12))	|| //REMOTE_START_ENABLED
																			((uint16_t)function_clear_faults_enabled				!= readFromFlash(13))	|| //CLEAR_FAULTS_ENABLED
																			((uint16_t)function_esc_tc_customizator_enabled			!= readFromFlash(14))	|| //ESC_TC_CUSTOMIZATOR_MASTER
																			((uint16_t)function_read_faults_enabled					!= readFromFlash(15))	|| //READ_FAULTS_ENABLED
																			((uint16_t)function_is_diesel_enabled					!= readFromFlash(16))	|| //IS_DIESEL
																			((uint16_t)function_regeneration_alert_enabled			!= readFromFlash(17))	){
																				//save it on flash
																				saveOnflash();
																		}
																		dashboard_menu_indent_level=0;
																		break;
																	case 1: //{'[',' ',']','S','t','a','r','t','&','S','t','o','p'},
																		function_smart_disable_start_stop_enabled=!function_smart_disable_start_stop_enabled;
																		requestToDisableStartAndStop=0;
																		break;
																	case 2: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-',},
																		break;
																	case 3: //{'[',' ',']','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',},
																		function_led_strip_controller_enabled = !function_led_strip_controller_enabled;
																		break;
																	case 4: //{'[',' ',']','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
																		function_shift_indicator_enabled=!function_shift_indicator_enabled;
																		break;
																	case 5: //{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',},
																		shift_threshold=shift_threshold+250;
																		if(shift_threshold>6000) shift_threshold=1500;
																		break;
																	case 6: //{'[',' ',']','M','y','2','3',' ','I','P','C', },
																		function_ipc_my23_is_installed=!function_ipc_my23_is_installed;
																		break;
																	case 7: //{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
																		function_regeneration_alert_enabled=!function_regeneration_alert_enabled;
																		//just for debug
																		//uint8_t tmpArr3[1]={BhBusChimeRequest}; //play sound
																		//addToUARTSendQueue(tmpArr3, 1);

																		break;
																	case 8: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-', },
																		break;
																	case 9: //{'[',' ',']','R','o','u','t','e',' ','M','e','s','s','a','g','e','s', },
																		function_route_msg_enabled=!function_route_msg_enabled;
																		break;
																	case 10: //{'[',' ',']','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',},
																		function_esc_tc_customizator_enabled = !function_esc_tc_customizator_enabled;
																		break;
																	case 11: //{'[',' ',']','D','y','n','o',},
																		function_dyno_mode_master_enabled=!function_dyno_mode_master_enabled;
																		break;
																	case 12: //{'[',' ',']','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
																		function_acc_virtual_pad_enabled=!function_acc_virtual_pad_enabled;
																		break;
																	case 13: //{'[',' ',']','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
																		function_front_brake_forcer_master=!function_front_brake_forcer_master;
																		break;
																	case 14: //{'[',' ',']','4','W','D',' ','D','i','s','a','b','l','e','r',},
																		function_4wd_disabler_enabled=!function_4wd_disabler_enabled;
																		break;
																	case 15: //{'[',' ',']','C','l','e','a','r',' ','F','a','u','l','t','s',},
																		function_clear_faults_enabled=!function_clear_faults_enabled;
																		break;
																	case 16: //{'[',' ',']','R','e','a','d',' ',' ','F','a','u','l','t','s',},
																		function_read_faults_enabled=!function_read_faults_enabled;
																		break;
																	case 17: //{'[',' ',']','R','e','m','o','t','e',' ','S','t','a','r','t',},
																		function_remote_start_Enabled=!function_remote_start_Enabled;
																		break;
																	case 18: //{'Ø',' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
																		function_is_diesel_enabled=!function_is_diesel_enabled;
																		break;
																	default:
																		break;
																}

																sendSetupDashboardPageToSlaveBaccable();

															}else{ //we want to return main menu
																dashboard_menu_indent_level=0;
																sendMainDashboardPageToSlaveBaccable(); //print menu
															}
														}



													}
													wheelPressedButtonID=0x10; //button released
													lastPressedWheelButtonDuration=0;
													lastPressedSpeedUpWheelButtonDuration=0;
													break;
												case 0x90: //RES button was pressed

													lastPressedWheelButtonDuration++;

													if (wheelPressedButtonID==0x10 && wheelPressedButtonID!=0x90){//we pressed RES for at least one instant
														wheelPressedButtonID=0x89; //avoid returning here until button is not released
													}
													if (wheelPressedButtonID==0x89 && (lastPressedWheelButtonDuration>50)){//we pressed RES for around 2 seconds, therefore we want to enable/disable Baccable menu on dashboard
														wheelPressedButtonID=0x90; //avoid returning here until button is not released

														baccableDashboardMenuVisible=!baccableDashboardMenuVisible; //toggle visualizazion of the menu

														if(!baccableDashboardMenuVisible){ //if menu needs to be hidden, print spaces to clear the string on dashboard
															clearDashboardBaccableMenu();
														}else{
															//dashboardPageIndex=0; //reset the page, just to be sure to show initial Baccable print
															//main_dashboardPageIndex=0; //shows initial baccable version
															//dashboard_menu_indent_level=0;
														}


													}
													break;
												case 0x12: //Cruise Control Disabled/Enabled
													break;
												default:
											}
										}

										if(cruiseControlDisabled && ACC_Disabled ){ //if we are allowed to use the buttons of the cruise control
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
									// pressStartButton
									#if defined(C1baccable)
										if(function_remote_start_Enabled==1){
											if(pressStartButton){
												if(rx_msg_header.DLC==8){
													// alter and send message to press car start button (maybe)
													rx_msg_data[0]=(rx_msg_data[0] & 0xF0) | 0x08; //set command ignition status=run
													uint8_t tmpCounter=(rx_msg_data[6]>>4)+1; //increment counter
													if (tmpCounter>0x0F) tmpCounter=0; //check counter
													rx_msg_data[6]= (rx_msg_data[6] & 0xF0) | tmpCounter; //assign counter
													rx_msg_data[7] = calculateCRC(rx_msg_data,8); //update checksum

													can_tx(&BODY4_msg_header, rx_msg_data); //send msg
													pressStartButton=0;
													onboardLed_blue_on();
												}
											}
										}
									#endif
									#if defined(C2baccable)
										//on C2 can bus, msg 0x384 contains, in byte3, bit6 contains left stalk button press status (LANE indicator button)
										if((rx_msg_data[3] & 0x40) ==0x40){ // left stalk button was pressed (lane following indicator)

											LANEbuttonPressLastTimeSeen=HAL_GetTick();//save current time it was pressed as LANEbuttonPressLastTimeSeen
											LANEbuttonPressCount++;
											if (LANEbuttonPressCount>8 ){ //8 is more or less 2 seconds
												ESCandTCinversion=!ESCandTCinversion; //toggle the status
												//if dyno is enabled or its change is in progress, avoid to switch ESP/TC.
												if(DynoModeEnabled || DynoStateMachine!=0xff) ESCandTCinversion=!ESCandTCinversion; //revert the change. won't do both things
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
									#if defined(C1baccable)
										if(function_led_strip_controller_enabled==1){
											if( (rx_msg_header.DLC==5) && (rx_msg_data[3]>=51) ){
												timeSinceLastReceivedAcceleratorMessage=HAL_GetTick();
												ledsStripIsOn=1;
												scaledVolume=scaleVolume(rx_msg_data[3]); //prendi il dato e scalalo, per prepararlo per l'invio alla classe vumeter
												vuMeterUpdate(scaledVolume,scaledColorSet);
											}
										}
									#endif
									break;
								case 0x0000041A:
									#if defined(C1baccable)
										if(rx_msg_header.DLC>=6){
											batteryStateOfCharge= (rx_msg_data[1] & 0b01111111); //set Most Significant Bit to zero
											batteryCurrent= (rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111));
										}
									#endif
									//battery state of charge is on byte 1 from bit 6 to 0 (Percentage)
									//battery current (A) is on byte 4 and in byte 5 from bit 7 to bit 4
									break;
								case 0x000004B1:
									#if defined(C1baccable)
										if(requestToDisableStartAndStop==1){//if requested, send message to simulate button press
											requestToDisableStartAndStop=0;
											startAndStopEnabled=0; //done
											memcpy(&disableStartAndStopMsgData, &rx_msg_data, rx_msg_header.DLC); //grab the message
											disableStartAndStopMsgHeader.DLC=rx_msg_header.DLC;
											//set message data, byte 5, bits from 5 to 3 to binary 001.
											disableStartAndStopMsgData[5]=(disableStartAndStopMsgData[5] & 0b11000111) | (0x01<<3);
											can_tx(&disableStartAndStopMsgHeader, disableStartAndStopMsgData);
											onboardLed_blue_on();

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
									#if defined(C1baccable)
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
									#if defined(C1baccable)
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
									#if defined(C1baccable)
										if((rx_msg_data[0]>>7)==1){
											cruiseControlDisabled=0;//disable additional parameter menu commands
											//onboardLed_blue_on();
										}else{
											cruiseControlDisabled=1;//enable additional parameter menu commands
											//onboardLed_red_on();
										}
									#endif
									break;
								case 0x000005AC:
									#if defined(BHbaccable)
										if(requestToPlayChime==1){  //if there is a request to play sound
											requestToPlayChime=0;
											//copy message
											memcpy(CHIME_msg_data, &rx_msg_data, rx_msg_header.DLC);
											//enable chime, by changing message
											CHIME_msg_header.DLC=rx_msg_header.DLC;
											CHIME_msg_data[0]= (CHIME_msg_data[0] & 0b00111111) ; //set bit 7 and 6 to zero (chime type 0)
											CHIME_msg_data[1]= (CHIME_msg_data[1] & 0b00111111) | 0b01000000; //byte1 bit 7 and 6 = 01 (seatbelt alarm active)
											CHIME_msg_data[3]=CHIME_msg_data[3] | 0xE0; //max volume
											can_tx(&CHIME_msg_header, CHIME_msg_data); //send msg
											onboardLed_blue_on();
										}
									#endif
									break;
								case 0x000005AE:
									#if defined(C1baccable)
										//this message is directed to IPC once per second. DPF status is on byte 4 bit 2. (1=dirty, 0=clean)
										if(rx_msg_header.DLC>=6){
											if(function_regeneration_alert_enabled){  //if out function was enabled in setup menu
												if(regenerationInProgress){ //if regeneration is in progress
													if(((rx_msg_data[4]>>2) & 0x01 )==0){ //if the message needs to be changed
														//change message and send it again
														memcpy(STATUS_ECM_msg_data, &rx_msg_data, rx_msg_header.DLC); //copy message
														STATUS_ECM_msg_data[4] |= 0x04; //DPF Dirty (bit 2) set to ON
														STATUS_ECM_msg_header.DLC=rx_msg_header.DLC;
														can_tx(&STATUS_ECM_msg_header, STATUS_ECM_msg_data); //send msg
														onboardLed_blue_on();
													}

												}
											}
											dieselEngineMode = (rx_msg_data[5]>>2 ) & 0b00000111 ;//byte 5 bit 4-2
										}
									#endif
									break;
								case 0x000005B0:
									#if defined(C2baccable)
										if((rx_msg_data[1] == 0x20) && ( DynoStateMachine == 0xff)){ // park assist button was pressed and there is no dyno Start sequence in progress
											ParkAssistButtonPressCount++;
											if (ParkAssistButtonPressCount>5){ // more or less 6 seconds
												ParkAssistButtonPressCount=0; //reset the count
												dynoToggle();
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
									#if defined(C1baccable)
										if(rx_msg_header.DLC>=8){
											switch ((rx_msg_data[7]>>4) & 0x07) {
												case 0x00: //ACC is off
													//onboardLed_red_on();
													ACC_Disabled=1; //enable additional parameter menu commands
													ACC_engaged=0; //acc not engaged
													break;
												case 0x02: //acc engaged
													ACC_engaged=1;
													ACC_Disabled=0; //disable additional parameter menu commands
													break;
												default:
													ACC_Disabled=0; //disable additional parameter menu commands
													ACC_engaged=0; //acc not engaged
											}
										}
									#endif
									//contains status of ACC on byte 7, from bit 6 to 4 (0=disabled, 1=enabled, 2=engaged 3=engaged brake only, 4=override, 5=cancel)
									break;
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

#if defined(C1baccable)
	void sendMainDashboardPageToSlaveBaccable(){
		uint8_t tmpStrLen=0;
		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus

		//update records if required
		switch(main_dashboardPageIndex){
			case 2: //READ_FAULTS_ENABLED
				if(function_read_faults_enabled==1){

				}
				break;
			case 3:
				if(function_clear_faults_enabled==1){
					if(clearFaultsRequest>0){
						memcpy(dashboard_main_menu_array[main_dashboardPageIndex], "WAIT...     ", 12);
						commandsMenuEnabled=0; //disable menu movement
					}else{
						memcpy(dashboard_main_menu_array[main_dashboardPageIndex], "CLEAR FAULTS", 12);
						commandsMenuEnabled=1; //enable menu movement
					}
				}
				break;
			case 4: //immo
				if(immobilizerEnabled){
					dashboard_main_menu_array[main_dashboardPageIndex][14]='N'; //on
					dashboard_main_menu_array[main_dashboardPageIndex][15]=' ';
				}else{
					dashboard_main_menu_array[main_dashboardPageIndex][14]='F'; //off
					dashboard_main_menu_array[main_dashboardPageIndex][15]='F';
				}
				break;
			case 5: //dyno
				break;
			case 6: //ESC/TC
				break;
			case 7: //front brake
				//update the string
				if(front_brake_forced==0){
					//update text {'F','r','o','n','t',' ','B','r','a','k','e',' ','N','o','r','m','a','l' },
					dashboard_main_menu_array[main_dashboardPageIndex][12]='N'; //normal
					dashboard_main_menu_array[main_dashboardPageIndex][15]='m';
					dashboard_main_menu_array[main_dashboardPageIndex][16]='a';
					dashboard_main_menu_array[main_dashboardPageIndex][17]='l';
				}else{
					//update text
					dashboard_main_menu_array[main_dashboardPageIndex][12]='F'; //forced
					dashboard_main_menu_array[main_dashboardPageIndex][15]='c';
					dashboard_main_menu_array[main_dashboardPageIndex][16]='e';
					dashboard_main_menu_array[main_dashboardPageIndex][17]='d';
				}
				break;
			case 8: //4wd
				//nothing to do
				break;
			default:
				//nothing to do
				break;
		}

		//add string to record
		switch(main_dashboardPageIndex){
			case 0:
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				if (tmpStrLen < DASHBOARD_MESSAGE_MAX_LENGTH) { //if required pad with spaces
					memset(&uartTxMsg[1+tmpStrLen], ' ', UART_BUFFER_SIZE-(1+tmpStrLen)); //set to zero remaining chars
				}
				break;
			default:
				memcpy(&uartTxMsg[1], dashboard_main_menu_array[main_dashboardPageIndex],UART_BUFFER_SIZE-1);
				break;
		}



		//send to slave baccable
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);

	}

	void sendSetupDashboardPageToSlaveBaccable(){
		//uint8_t tmpStrLen=0;
		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus

		//update records if required
		switch(setup_dashboardPageIndex){
			case 0: //{'S','A','V','E','&','E','X','I','T',},
				break;
			case 1: //{'[',' ',']','S','t','a','r','t','&','S','t','o','p'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_smart_disable_start_stop_enabled];
				break;
			case 2: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-',},
				break;
			case 3: //{'[',' ',']','L','e','d',' ','C','o','n','t','r','o','l','l','e','r',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_led_strip_controller_enabled];
				break;
			case 4: //{'[',' ',']','S','h','i','f','t',' ','I','n','d','i','c','a','t','o','r'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_shift_indicator_enabled];
				break;
			case 5: //{'S','h','i','f','t',' ','R','P','M',' ','3','0','0','0',},
				char tmpfloatString[5];
				floatToStr(tmpfloatString,(float)shift_threshold,0,5);
				dashboard_setup_menu_array[setup_dashboardPageIndex][10]=tmpfloatString[0];
				dashboard_setup_menu_array[setup_dashboardPageIndex][11]=tmpfloatString[1];
				dashboard_setup_menu_array[setup_dashboardPageIndex][12]=tmpfloatString[2];
				dashboard_setup_menu_array[setup_dashboardPageIndex][13]=tmpfloatString[3];
				break;
			case 6: //{'[',' ',']','M','y','2','3',' ','I','P','C', },
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_ipc_my23_is_installed];
				break;
			case 7: //{'O',' ',' ','R','e','g','e','n','.',' ','A','l','e','r','t',' ',' ',' '},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_regeneration_alert_enabled];
				break;
			case 8: //{'[',' ',']','-','-','-','-','-','-','-','-','-','-','-', },
				break;
			case 9: //{'[',' ',']','R','o','u','t','e',' ','M','e','s','s','a','g','e','s', },
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_route_msg_enabled];
				break;
			case 10: //{'[',' ',']','E','S','C','/','T','C',' ','C','u','s','t','o','m','.',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_esc_tc_customizator_enabled];
				break;
			case 11: //{'[',' ',']','D','y','n','o',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_dyno_mode_master_enabled];
				break;
			case 12: //{'[',' ',']','A','C','C',' ','V','i','r','t','u','a','l',' ','P','a','d'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_acc_virtual_pad_enabled];
				break;
			case 13: //{'[',' ',']','B','r','a','k','e','s',' ','O','v','e','r','r','i','d','e'},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_front_brake_forcer_master];
				break;
			case 14: //{'[',' ',']','4','W','D',' ','D','i','s','a','b','l','e','r',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_4wd_disabler_enabled];
				break;
			case 15: //{'[',' ',']','C','l','e','a','r',' ','F','a','u','l','t','s',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_clear_faults_enabled];
				break;
			case 16: //{'[',' ',']','R','e','a','d',' ',' ','F','a','u','l','t','s',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_read_faults_enabled];
				break;
			case 17: //{'[',' ',']','R','e','m','o','t','e',' ','S','t','a','r','t',},
				dashboard_setup_menu_array[setup_dashboardPageIndex][0]=checkbox_symbols[function_remote_start_Enabled];
				break;
			case 18: //{'Ø',' ',' ','D','i','e','s','e','l',' ',' ',' ','P','a','r','a','m','s'},
				if(function_is_diesel_enabled){
					dashboard_setup_menu_array[setup_dashboardPageIndex][3] ='D';
					dashboard_setup_menu_array[setup_dashboardPageIndex][4] ='i';
					dashboard_setup_menu_array[setup_dashboardPageIndex][5] ='e';
					dashboard_setup_menu_array[setup_dashboardPageIndex][6] ='s';
					dashboard_setup_menu_array[setup_dashboardPageIndex][7] ='e';
					dashboard_setup_menu_array[setup_dashboardPageIndex][8] ='l';
					dashboard_setup_menu_array[setup_dashboardPageIndex][9] =' ';
					dashboard_setup_menu_array[setup_dashboardPageIndex][10]=' ';
				}else{
					dashboard_setup_menu_array[setup_dashboardPageIndex][3] ='G';
					dashboard_setup_menu_array[setup_dashboardPageIndex][4] ='a';
					dashboard_setup_menu_array[setup_dashboardPageIndex][5] ='s';
					dashboard_setup_menu_array[setup_dashboardPageIndex][6] ='o';
					dashboard_setup_menu_array[setup_dashboardPageIndex][7] ='l';
					dashboard_setup_menu_array[setup_dashboardPageIndex][8] ='i';
					dashboard_setup_menu_array[setup_dashboardPageIndex][9] ='n';
					dashboard_setup_menu_array[setup_dashboardPageIndex][10]='e';
				}
				break;

			default:
				break;
		}

		memcpy(&uartTxMsg[1], &dashboard_setup_menu_array[setup_dashboardPageIndex],UART_BUFFER_SIZE-1);
		//send to slave baccable
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);


	}

	void sendDashboardPageToSlaveBaccable(float param){
		uint8_t tmpStrLen=0;
		uint8_t tmpStrLen2=0;
		uint8_t tmpStrLen3=0;

		uartTxMsg[0]= BhBusIDparamString;//first char shall be a # to talk with slave canable connected to BH can bus

		switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId){ //do preliminary additional stuff for special parameters (not uds)
			case 0x10: //print oil pressure
				param=(float) oilPressure * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale;
				break;
			case 0x11: //power in CV
				param=((float)torque + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyValOffset) * (float)currentRpmSpeed * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale ;
				break;
			case 0x12: //torque in NM
				param=((float)torque  * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x13: //battery state of charge (%)
				param=(float)batteryStateOfCharge  * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale;
				break;
			case 0x14: //battery current (A)
				param=((float)batteryCurrent  * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x15: //engine oil temperature
				param=((float)oilTemperature * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x16: //transmission temperature
				param=((float)transmissionTemperature * uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScale) + uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyScaleOffset;
				break;
			case 0x17: //current gear
				param=(float)(currentGear>>4);
				break;
			case 0x18: //current speed (km/h)
				param=currentSpeed_km_h;
				break;
			//case 0x19: //Diesel Engine Mode
			//	param=dieselEngineMode;
			//	break;
			default:
				break;
		}


		switch(uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId){
			case 0: //print baccable menu
				tmpStrLen=strlen(FW_VERSION);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH;
				memcpy(&uartTxMsg[1],FW_VERSION,tmpStrLen);
				break;
			case 0x19:
				if (uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId == 0x19) { //if diesel engine mode status
					tmpStrLen=18;
					switch(dieselEngineMode){
						case 0:
							memcpy(&uartTxMsg[1], "REGEN.: NONE      ", tmpStrLen);
							break;
						case 1:
							memcpy(&uartTxMsg[1], "REGEN.: DPF LO    ", tmpStrLen);
							break;
						case 2:
							memcpy(&uartTxMsg[1], "REGEN.: DPF HI    ", tmpStrLen);
							break;
						case 3:
							memcpy(&uartTxMsg[1], "REGEN.: NSC De-NOx", tmpStrLen);
							break;
						case 4:
							memcpy(&uartTxMsg[1], "REGEN.: NSC De-SOx", tmpStrLen);
							break;
						case 5:
							memcpy(&uartTxMsg[1], "REGEN.: SCR HeatUp", tmpStrLen);
							break;
						default:
							memcpy(&uartTxMsg[1], "REGEN.: NONE.     ", tmpStrLen);
							break;
					}
				}
				break;
			default:
				tmpStrLen=strlen((const char *)uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name);
				if(tmpStrLen>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen=DASHBOARD_MESSAGE_MAX_LENGTH; //truncate it. no space left
				memcpy(&uartTxMsg[1], &uds_params_array[function_is_diesel_enabled][dashboardPageIndex].name,tmpStrLen); //prepare name of parameter
				if(param!=-3400000000000000000){ //if different than special value (since special value means no value to send)
					//scale param still done, we don't need to do it here

					//convert param from float to string
					char tmpfloatString[10];

					if (uds_params_array[function_is_diesel_enabled][dashboardPageIndex].reqId == 0x17) { //if Current gear request data - currentGear

						if ((uint8_t)param<11){
							tmpfloatString[0]=gearArray[(uint8_t)param];
						}else{
							tmpfloatString[0] = '-';
						}

						tmpfloatString[1] =0;
					}else{
						floatToStr(tmpfloatString,param,uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyDecimalDigits,sizeof(tmpfloatString));
					}

					//add param to the page String
					tmpStrLen2=strlen(tmpfloatString);
					if(tmpStrLen+tmpStrLen2>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen2=DASHBOARD_MESSAGE_MAX_LENGTH-tmpStrLen; //truncate it. no space left
					memcpy(&uartTxMsg[1+tmpStrLen],tmpfloatString,tmpStrLen2);

					//float tmpVal9=200000.45601928209374; ///ADDED FOR TEST.......
					//char *tmpStr9=(char*)malloc(10);

					//floatToStr(tmpfloatString,param,2,sizeof(tmpfloatString));
					//tmpStrLen2=strlen(tmpfloatString);
					//memcpy(&dashboardPageStringArray[tmpStrLen],tmpfloatString,tmpStrLen2);
				}
				//add measurement unit
				tmpStrLen3=strlen((const char *)uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyMeasurementUnit);
				if(tmpStrLen+tmpStrLen2+tmpStrLen3>DASHBOARD_MESSAGE_MAX_LENGTH) tmpStrLen3=DASHBOARD_MESSAGE_MAX_LENGTH-tmpStrLen-tmpStrLen2; //truncate it. no space left
				memcpy(&uartTxMsg[1+tmpStrLen+tmpStrLen2],&uds_params_array[function_is_diesel_enabled][dashboardPageIndex].replyMeasurementUnit,tmpStrLen3);
		}
		if (tmpStrLen+tmpStrLen2+tmpStrLen3 < DASHBOARD_MESSAGE_MAX_LENGTH) { //if required pad with zeros
			memset(&uartTxMsg[1+tmpStrLen+tmpStrLen2+tmpStrLen3], ' ', UART_BUFFER_SIZE-(1+tmpStrLen+tmpStrLen2+tmpStrLen3)); //set to zero remaining chars
		}
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);


	}

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

	uint8_t saveOnflash(){ //store params permanently on flash
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

		//it seems that stm32F072 supports only writing 2byte words
		//write parameter
		uint16_t params[20] = {
		  immobilizerEnabled,
		  function_smart_disable_start_stop_enabled,
		  function_led_strip_controller_enabled,
		  function_shift_indicator_enabled,
		  shift_threshold,
		  function_ipc_my23_is_installed,
		  function_route_msg_enabled,
		  function_dyno_mode_master_enabled,
		  function_acc_virtual_pad_enabled,
		  function_front_brake_forcer_master,
		  function_4wd_disabler_enabled,
		  function_remote_start_Enabled,
		  function_clear_faults_enabled,
		  function_esc_tc_customizator_enabled,
		  function_read_faults_enabled,
		  function_is_diesel_enabled,
		  function_regeneration_alert_enabled,
		};

		for (uint8_t i = 0; i < 17; i++) {
		    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, LAST_PAGE_ADDRESS + (i * 4), params[i]) != HAL_OK) {
		        HAL_FLASH_Lock();
		        onboardLed_red_blink(9);
		        return 255;
		    }
		}

		//lock the flash
		HAL_FLASH_Lock();
		return 0;
	}

	uint16_t readFromFlash(uint8_t paramId){
		if(paramId<1) return 0;
		uint16_t tmpParam=*(volatile uint16_t*)(LAST_PAGE_ADDRESS+((paramId-1)*4));
		switch(paramId){
			case 1: //IMMOBILIZER_ENABLED (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(IMMOBILIZER_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 2: //SMART_DISABLE_START_STOP enable status (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(SMART_DISABLE_START_STOP)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 3: //LED_STRIP_CONTROLLER_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(LED_STRIP_CONTROLLER_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 4: //SHIFT_INDICATOR_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(SHIFT_INDICATOR_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 5: //SHIFT_THRESHOLD
				if(tmpParam==0xFFFF){
					#if defined(SHIFT_THRESHOLD)
						return SHIFT_THRESHOLD;
					#else
						return 3500; // another default value
					#endif
				}
				break;
			case 6: //IPC_MY23_IS_INSTALLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(IPC_MY23_IS_INSTALLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 7: //ROUTE_MSG  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ROUTE_MSG)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 8: //DYNO_MODE_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(DYNO_MODE_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 9: //ACC_VIRTUAL_PAD  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ACC_VIRTUAL_PAD)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 10: //FRONT_BRAKE_FORCER_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(FRONT_BRAKE_FORCER_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 11: //_4WD_DISABLER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(_4WD_DISABLER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 12: //REMOTE_START_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(REMOTE_START_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 13: //CLEAR_FAULTS_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(CLEAR_FAULTS_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 14: //ESC_TC_CUSTOMIZATOR_MASTER  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(ESC_TC_CUSTOMIZATOR_MASTER)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 15: //READ_FAULTS_ENABLED  (1=enabled 0=disabled)
				if(tmpParam>1){
					#if defined(READ_FAULTS_ENABLED)
						return 1;
					#else
						return 0;
					#endif
				}
				break;
			case 16: //IS_DIESEL  (1=enabled 0=disabled)
					if(tmpParam>1){
						#if defined(IS_DIESEL)
							return 1;
						#else
							return 0;
						#endif
					}
					break;
			case 17: //REGENERATION_ALERT_ENABLED  (1=enabled 0=disabled)
					if(tmpParam>1){
						#if defined(REGENERATION_ALERT_ENABLED)
							return 1;
						#else
							return 0;
						#endif
					}
					break;
			default:
				return 0;
				break;
		}
		return tmpParam;
	}

	void clearDashboardBaccableMenu(){
		//prepare empty message
		uartTxMsg[0]=BhBusIDparamString; //# to send message to baccable slave connected to BH can bus
		for(uint8_t i=1;i<UART_BUFFER_SIZE;i++){
			uartTxMsg[i]=0x20; //space char
		}
		//send it
		addToUARTSendQueue(uartTxMsg, UART_BUFFER_SIZE);

	}
#endif

#if defined(C2baccable)
	void dynoToggle(){
		if(DynoStateMachine == 0xff){ // there is no dyno Start sequence in progress
			DynoStateMachine=0; //state machine
			ESCandTCinversion=0; //do not change ESC and TC if dynomode is requested
			DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1; //length of DIAGNOSTIC SESSION msg
			can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
			onboardLed_blue_on();
			DynoStateMachineLastUpdateTime=HAL_GetTick();//save last time seen
			//wait the feedback from ECU
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

uint8_t calculateCRC(uint8_t* data, uint8_t arraySize) {
	uint8_t crc = 0xFF;
	if(arraySize>1){
		//calculate sae_j1850 CRC-8 of the array (excluded last element, that will be used to store the final CRC
		for (uint8_t i=0;i<arraySize-1;i++){
			crc ^= data[i];
			for (int i = 0; i < 8; ++i){
				crc = (crc & 0x80) ? (crc << 1) ^ 0x1D : crc << 1;
			}
		}
		return (crc ^ 0xFF); //return calculated checksum
	}
	return 0; //nothing to calculate
}


#if defined(STM32F072xB)
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
#elif defined(STM32G431xx)
void SystemClock_Config(void){
    HAL_Init();

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Configure the main internal regulator output voltage
    */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      while(1);
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_8) != HAL_OK)
    {
      while(1);
    }
    /** Initializes the peripherals clocks
    */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      while(1);
    }

    /** Configures CRS
         */
    RCC_CRSInitTypeDef pInit = {0};
	pInit.Prescaler = RCC_CRS_SYNC_DIV1;
	pInit.Source = RCC_CRS_SYNC_SOURCE_USB;
	pInit.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
	pInit.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
	pInit.ErrorLimitValue = 34;
	pInit.HSI48CalibrationValue = 32;

	HAL_RCCEx_CRSConfig(&pInit);

    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE(); // just nrst is on portG

    __option_byte_config();
}

static void __option_byte_config(void);

// Configure option bytes: set BoR level to 4 (2.8v)
static void __option_byte_config(void){
	// Set BoR level to 4
	// This eliminates an issue where poor quality USB hubs
	// that provide low voltage before switching the 5v supply on
	// which was causing PoR issues where the microcontroller
	// would enter boot mode incorrectly.

	// Get option bytes
	FLASH_OBProgramInitTypeDef config = {0};
	HAL_FLASHEx_OBGetConfig(&config);

	// If BoR is already at level 4, then don't bother doing anything
	if((config.USERConfig & FLASH_OPTR_BOR_LEV_Msk) == OB_BOR_LEVEL_4)
		return;

	// Unlock flash
	if(HAL_FLASH_Unlock() != HAL_OK)
		return;

	// Unlock option bytes
   if(HAL_FLASH_OB_Unlock() != HAL_OK)
	   return;

   FLASH_OBProgramInitTypeDef progval = {0};

   // Update the user option byte
   progval.OptionType = OPTIONBYTE_USER;
   progval.USERType = OB_USER_BOR_LEV;
   progval.USERConfig = OB_BOR_LEVEL_4; // 2.8v for level 4

   // Program the option bytes
   HAL_FLASHEx_OBProgram(&progval);

   // Lock flash / option bytes
   HAL_FLASH_OB_Lock();
   HAL_FLASH_Lock();

   // Note: option byte update will take effect on the next power cycle
}
#endif

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
