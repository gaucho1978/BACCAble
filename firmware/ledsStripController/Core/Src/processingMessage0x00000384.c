/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingMessage0x00000384.h"

void processingMessage0x00000384(){
	// pressStartButton
	#if defined(C1baccable)
		if(currentDNAmode!=(rx_msg_data[1]& 0x7C)){ //RDNA mode was changed, reset the ESCandTCinversion
			ESCandTCinversion=0;
		}
		//current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08], 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
		currentDNAmode=rx_msg_data[1] & 0x7C; //7C is the mask from bit 6 to 2 (we avoid bit shift to save cpu loops)
		if (ESCandTCinversion){

			if(currentDNAmode!=0x30){ //if not in race
				rx_msg_data[1] = (rx_msg_data[1] & ~0x7C) | 0x30;  //set Race mode (0x30) to show on IPC the race screen
			}
			//uint8_t tmpCounter=(rx_msg_data[6] & 0x0F)+1;
			//if(tmpCounter>0x0F) tmpCounter=0;
			//rx_msg_data[6]= (rx_msg_data[6] & 0xF0) | tmpCounter;   //increment counter
			rx_msg_data[7]=calculateCRC(rx_msg_data,rx_msg_header.DLC); //update CRC
			can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
			//onboardLed_blue_on();
		}
		//memcpy(&DNA_msg_data, &rx_msg_data, 8);

	#endif

	#if defined(C2baccable)
		//on C2 can bus, msg 0x384 contains, in byte3, bit6 contains left stalk button press status (LANE indicator button)
		if((rx_msg_data[3] & 0x40) ==0x40){ // left stalk button was pressed (lane following indicator)

			LANEbuttonPressLastTimeSeen=currentTime;//save current time it was pressed as LANEbuttonPressLastTimeSeen
			LANEbuttonPressCount++;
			if (LANEbuttonPressCount>8 ){ //8 is more or less 2 seconds
				ESCandTCinversion=!ESCandTCinversion; //toggle the status
				//if dyno is enabled or its change is in progress, avoid to switch ESP/TC.
				if(DynoModeEnabled || DynoStateMachine!=0xff) ESCandTCinversion=!ESCandTCinversion; //revert the change. won't do both things

				if(ESCandTCinversion && function_show_race_mask){ //if enabled, notify C1 and BH
					uint8_t tmpArr1[2]={C1_Bh_BusID, C1BHcmdShowRaceScreen};
					addToUARTSendQueue(tmpArr1, 2);
				}else{
					uint8_t tmpArr1[2]={C1_Bh_BusID, C1BHcmdStopShowRaceScreen};
					addToUARTSendQueue(tmpArr1, 2);
				}

				onboardLed_blue_on();
				LANEbuttonPressCount=0; //reset the count
			}
		}else{
			if(currentTime-LANEbuttonPressLastTimeSeen>1000){ // if LANEbuttonPressLastTimeSeen, is older than 1 second ago, it means that button was released
				LANEbuttonPressCount=0;// reset the count assigning it zero
			}
		}

		if(currentDNAmode!=(rx_msg_data[1]& 0x7C)){ //RDNA mode was changed, reset the ESCandTCinversion
			ESCandTCinversion=0;
		}
		//current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 1 from bit 6 to bit 2 (0x0=Natural [shifted by 2 bits becomes 0x00], 0x2=dynamic [shifted by 2 bits becomes 0x08], 0x4=AllWeather [shifted by 2 bits becomes 0x10], 0xC=race [shifted by 2 bits becomes 0x30]
		currentDNAmode=rx_msg_data[1] & 0x7C; //7C is the mask from bit 6 to 2 (we avoid bit shift to save cpu loops)
		if (ESCandTCinversion){
			//memcpy(&DNA_msg_data, &rx_msg_data, 8);
			if(currentDNAmode==0x30){ //race
				//DNA_msg_data[1]= (DNA_msg_data[1] & ~0x7C) | (0x08 & 0x7C); //set Dynamic mode (0x08) to enable ESC and TC
				rx_msg_data[1]= (rx_msg_data[1] & ~0x7C) | 0x08; //set Dynamic mode (0x08) to enable ESC and TC
			}else{
				//DNA_msg_data[1] = (DNA_msg_data[1] & ~0x7C) | (0x30 & 0x7C);  //set Race mode (0x30) to disable ESC and TC
				rx_msg_data[1] = (rx_msg_data[1] & ~0x7C) | 0x30;  //set Race mode (0x30) to disable ESC and TC
			}
			//uint8_t tmpCounter=(rx_msg_data[6] & 0x0F)+1;
			//if(tmpCounter>0x0F) tmpCounter=0;
			//rx_msg_data[6]= (rx_msg_data[6] & 0xF0) | tmpCounter;   //increment counter
			//rx_msg_data[7]=calculateCRC(rx_msg_data,rx_msg_header.DLC); //update CRC
			can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //transmit the modified packet
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


}
