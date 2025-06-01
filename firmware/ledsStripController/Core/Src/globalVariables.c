/*
 * globalVariables.c
 *
 *  Created on: Apr 27, 2025
 *      Author: GauchoHP
 */
#include "globalVariables.h"


const char *FW_VERSION=_FW_VERSION;

// force print
#pragma message ("FW_VERSION: " _FW_VERSION)

#if defined(UCAN_BOARD_LED_INVERSION)
	const uint8_t led_light_on_bit=1;
#else
	const uint8_t led_light_on_bit=0;
#endif



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
	uint16_t engineOnSinceMoreThan5seconds=0; //if>=500 means engine is on since at least 5 seconds (for RFHUB too)


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
			{'A','s','s','i','s','t',' ','T','o','r','q','u','e',' ','1','0','0','N'},
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
											{.name={'R','E','G','E','N',':',' ',' ',' ',' ',' ',' ',},			.reqId=0x19,		.reqLen=4,  .reqData=SWAP_UINT32(0x00000000),	.replyId=0x000005AE,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,				.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
											{.name={'L','A','S','T',' ','R','E','G','E','N','.',':',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223807),	.replyId=0x18DAF110,	.replyLen=3,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.1,			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
											{.name={'T','O','T',' ','R','E','G','E','N',':',' ',},				.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x032218A4),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={}								},
											{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x03223809),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=1,  			.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'k','m',}						},
											{.name={'M','E','A','N',' ','R','E','G','E','N',':',' ',},			.reqId=0x18DA10F1,  .reqLen=4,  .reqData=SWAP_UINT32(0x0322380A),	.replyId=0x18DAF110,	.replyLen=2,	.replyOffset=0,	.replyValOffset=0,		.replyScale=0.01666666666,	.replyScaleOffset=0,	.replyDecimalDigits=0,	.replyMeasurementUnit={'m','i','n',}					},
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

	CAN_TxHeaderTypeDef uds_parameter_request_msg_header={.IDE=CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId=0x18DA10F1, .DLC=3};
	uint8_t baccableDashboardMenuVisible=0;
	uint8_t baccabledashboardMenuWasVisible=0; //tells us if menu was previously disabled (and then when motor will turn we want to show it again
	uint8_t oilPressure; //oil pressure without scaling (this value shall be multiplied by xx to obtain value in bar).
	uint8_t oilTemperature; //oil temperature in celsious degrees (to correct by offset)
	uint16_t torque; //torque
	uint8_t batteryStateOfCharge=0; //battery charge %
	uint16_t batteryCurrent; //battery current (to be converted in Amps)
	uint8_t transmissionTemperature;
	uint8_t uds_parameter_request_msg_data[8];
	uint8_t dashboardPageIndex=0; //to send message index - it changes when you press cruise control buttons
	uint32_t last_sent_uds_parameter_request_Time=0; //stores last time we send a uds parameter request

	uint8_t dieselEngineRegenerationMode=0; //0=None, 1=DPF_REGEN_LO, 2=DPF_REGEN_HI, 3=NSC_DE_NOX_REGEN, 4=NSC_DE_SOX_REGEN, 5=SCR_HEATUP_STRATEGY

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
	uint32_t lastSentTelematic_display_info_msg_Time=0;
	uint8_t telematic_display_info_field_totalFrameNumber=(DASHBOARD_MESSAGE_MAX_LENGTH / 3) - 1; //it shall be a multiple of 3 reduced by 1 (example: 3x2-1=5)
	uint8_t telematic_display_info_field_frameNumber=0; //current frame
	uint8_t telematic_display_info_field_infoCode=0x09;
	uint8_t paramsStringCharIndex=0; // next char to send index
	CAN_TxHeaderTypeDef telematic_display_info_msg_header={.IDE=CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId=0x090, .DLC=8};
	uint8_t telematic_display_info_msg_data[8];
	uint8_t requestToSendOneFrame=0; //set to 1 to send one frame on dashboard

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
uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH]={' ',}; //it contains string to print on dashboard

float currentSpeed_km_h=0; //current vehicle speed

uint32_t weCanSendAMessageReply=0; //defines last time that C2 or BH baccable received a message (used by C2 and BH baccable)
uint8_t uartTxMsg[UART_BUFFER_SIZE]; // it contains string to send over uart
uint32_t currentTime; //stores current time in milliseconds, each time we enter the main loop

UART_HandleTypeDef huart2; // this is the serial line between baccables

uint32_t currentRpmSpeed=0;	//used by C1baccable
uint8_t currentGear=0; 		//used by C1baccable

// Storage for status and received message buffer
CAN_RxHeaderTypeDef rx_msg_header;  //msg header
uint8_t rx_msg_data[8] = {0,};  //msg data

uint8_t msg_buf[SLCAN_MTU]; //msg converted in ascii to send over usb

uint8_t _4wd_disabled=0; //if =4 disables 4wd
uint8_t front_brake_forced=0; //if=5 disables Front brakes
uint8_t commandsMenuEnabled=1; //if 0 disables the up-down buttons to change menu position
