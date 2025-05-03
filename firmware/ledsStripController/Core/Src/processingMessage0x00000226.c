/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingMessage0x00000226.h"

void processingMessage0x00000226(){
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

}
