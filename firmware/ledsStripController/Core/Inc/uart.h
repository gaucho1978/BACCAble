#ifndef _UART_H
#define _UART_H


	#include "stm32f0xx_hal.h"


	#include "onboardLed.h"
	#include "string.h"
	//#include "error.h"
		
	
	#define C1BusID					0x01 //first byte sent over uart to identify destinator baccable connected to C1 Can Bus
	#define C2BusID					0x02 //first byte sent over uart to identify destinator baccable connected to C2 Can Bus
	#define BhBusIDparamString		0x03 //first byte sent over uart to identify destinator baccable connected to BH Can Bus to transfer parameter string
	#define AllSleep				0x04 //first byte sent over uart to tell anyone connecter to go to sleep (or low consumption).
	#define C2BusIDAllSleepAck		0x05 //first byte sent over uart by C2 baccable in order to communicate that the sleep was received and executed
	#define BHBusIDAllSleepAck		0x06 //first byte sent over uart by BH baccable in order to communicate that the sleep was received and executed
	#define AllResetFaults			0x07 //first byte sent over uart by C1 baccable in order to communicate that the Reset Faults is required
	#define BhBusIDgetStatus		0x08 //first byte sent over uart to identify destinator baccable connected to BH Can Bus to request its status
	#define BhBusChimeRequest		0x09 //first byte sent over uart to indentify message to BH to play sound
	#define BhBusID					0x0A //first byte sent over uart to indentify destinator baccable connected to BH Can Bus
	#define C2_Bh_BusID				0x0B //first byte sent over uart to indentify destinator baccable connected to C2 and BH Can Bus
	#define C1_Bh_BusID				0x0C //first byte sent over uart to indentify destinator baccable connected to C1 and BH Can Bus

	#define C2cmdtoggleDyno					0x20 //second byte of the message to C2 bus, identifies the request to toggle dyno
	#define C2cmdNormalFrontBrake			0x21 //second byte of the message to C2 bus, identifies the request to set front Brake to normal
	#define C2cmdForceFrontBrake			0x22 //second byte of the message to C2 bus, identifies the request to force Front Brake ON
	#define C2cmdGetStatus					0x23 //second byte of the message to C2 bus, identifies the request to getStatus
	#define C2cmdtoggleEscTc				0x24 //second byte of the message to C2 bus, identifies the request to toggle ESC/TC
	#define C2cmdDynoActive					0x25 //second byte of the message to C2 bus, identifies the status dyno Active
	#define C2cmdDynoNotActive				0x26 //second byte of the message to C2 bus, identifies the status dyno Not Active

	#define C2_Bh_cmdSetPedalBoostStatus	0x27 //second byte of the message to C2 and BH bus, identifies the pedal booster function status. Third byte of the message will contain its status

	#define BHcmdOdometerBlinkDisable		0x20 //second byte of the message to BH bus, identifies the request to disable odometer blink
	#define BHcmdOdometerBlinkDefault		0x21 //second byte of the message to BH bus, identifies the request to restore normal odometer blink status

	#define C1BHcmdShowRaceScreen			0x22 //second byte of the message to C1 and BH bus, identifies the request to show Race Screen on dashboard
	#define C1BHcmdStopShowRaceScreen		0x23 //second byte of the message to C1 and BH bus, identifies the request to STOP to show Race Screen on dashboard


	#define UART_BUFFER_SIZE DASHBOARD_MESSAGE_MAX_LENGTH + 1


	
	void uart_init();
	//void uart_transmit_data(char*  message);
	void process_received_data();
	void enter_standby_mode();
	void resetOtherProcessorsSleepStatus();
	uint8_t getOtherProcessorsSleepingStatus();
	void addToUARTSendQueue(const uint8_t *data, size_t length);
	void processUART();
#endif
