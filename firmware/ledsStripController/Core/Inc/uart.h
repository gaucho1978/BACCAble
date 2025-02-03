#ifndef _UART_H
#define _UART_H
	#include "stm32f0xx_hal.h"
	#include "onboardLed.h"
	#include "string.h"
	//#include "error.h"
		
	
	#define C1BusID 0x01 //identifies the first byte sent over uart to identify destinator baccable connected to C1 Can Bus
	#define C2BusID 0x02 //identifies the first byte sent over uart to identify destinator baccable connected to C2 Can Bus
	#define BhBusID 0x03 //identifies the first byte sent over uart to identify destinator baccable connected to BH Can Bus
	#define UART_BUFFER_SIZE 19

	
	void uart_init();
	//void uart_transmit_data(char*  message);
	void process_received_data();
	
	
#endif
