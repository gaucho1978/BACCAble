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


	#define UART_BUFFER_SIZE 19

	
	void uart_init();
	//void uart_transmit_data(char*  message);
	void process_received_data();
	void enter_standby_mode();
	void resetOtherProcessorsSleepStatus();
	uint8_t getOtherProcessorsSleepingStatus();
	
#endif
