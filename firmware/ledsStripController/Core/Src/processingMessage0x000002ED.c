/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingMessage0x000002ED.h"

void processingMessage0x000002ED(){
	#if defined(C1baccable)
		waterTemperature= rx_msg_data[0];

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

}
