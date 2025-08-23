/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingMessage0x000001EF.h"

void processingMessage0x000001EF(){
	#if defined(C1baccable)
		if(rx_msg_header.DLC==8){

			//avoid thief to simulate engine rotation. As soon as RFHUB tells ignition is not in RUN reset engineOn status stored inside baccable
			if((rx_msg_data[0] & 0x0F)!=0x08) engineOnSinceMoreThan5seconds=0;

			if(function_close_windows_with_door_lock){

				if(rx_msg_data[2]>>4==0x1){ //it is the message to lock the car.
					RF_fob_number=rx_msg_data[1] & 0x1E; //store fob id (the real id is obtainable by shifting by 1 bit to the right, but this value is better, in order to be used in the next message)
					RF_requestor=rx_msg_data[2] & 0x0E; //store requestor ID (real ID is obtainable by shifting by 1 bit to the right, but this value is better, in order to be used in the next message)
					//within 1 second, send request to close windows
					doorCloseTime=currentTime;
					closeWindowsRequest=1;
				}

				if((rx_msg_data[2]>>4==0x4) || (rx_msg_data[2]>>4==0x3)){ //it is the message to open the car
					closeWindowsRequest=0; //interrupt the action, if in progress
				}

				if(closeWindowsRequest==2){ //we have to set the windows Ajar
					//send message to close the windows
					rx_msg_data[1]= rx_msg_data[1] | RF_fob_number; //set proper key fob
					rx_msg_data[1]= rx_msg_data[1] & ~0x01; //set request to open all windows (set first bit to 0)
					rx_msg_data[2]= (rx_msg_data[2] & 0x0F) | 0xB0;
					rx_msg_data[2]= rx_msg_data[2] | RF_requestor; //set requestor
					rx_msg_data[2]= rx_msg_data[2] & 0x0F; //set requesto to close all windows
					rx_msg_data[7] = calculateCRC(rx_msg_data,rx_msg_header.DLC); //update checksum
					can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send msg
					if(currentTime-doorCloseTime>7190){ //if at least 190msec from windows closed is passed, windows shoud be opened for at least 2 centimeters
						closeWindowsRequest=0; //task completed
					}
				}

				if(closeWindowsRequest==1){ //we have to close the windows
					if(currentTime-doorCloseTime>1000){ //if at least one second from door closure is passed
						//send message to close the windows
						rx_msg_data[1]= rx_msg_data[1] | RF_fob_number; //set proper key fob
						rx_msg_data[1]= rx_msg_data[1] | 0x01; //set request to close all windows
						rx_msg_data[2]= rx_msg_data[2] | RF_requestor; //set requestor
						rx_msg_data[2]= rx_msg_data[2] & 0x0F; //set requesto to close all windows
						rx_msg_data[7] = calculateCRC(rx_msg_data,rx_msg_header.DLC); //update checksum
						can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send msg

						if(currentTime-doorCloseTime>7000){ //after 6 seconds of windows movement, they should be closed
							closeWindowsRequest=0;
							if(function_close_windows_with_door_lock==2){ //if windows Ajar is selected,
								closeWindowsRequest=2; //request the windows ajar
							}
						}
					}
				}

			}



			/*
			if(function_remote_start_Enabled==1){
				if(rx_msg_data[2]>>4==0x4){ //it is the message to open the car. THIS IS JUST FOR TEST!!!!!
					RF_fob_number=rx_msg_data[1] & 0x1E; //store fob id (the real id is obrainable by shifting by 1 bit to the right, but this value is better, in order to be used in the next message
					//within 2 seconds, send remote start
					doorOpenTime=currentTime;
					engineRemoteStartRequest=3;
				}

				if(engineRemoteStartRequest){
					//if(doorOpenTime+2000<currentTime){ //we have to start the engine
					if(currentTime-doorOpenTime>2000){ //we have to start the engine
						memcpy(&REMOTE_START_msg_data, &rx_msg_data, 8);
						//REMOTE_START_msg_data[0]= (REMOTE_START_msg_data[0] & 0x0F ) | 0x80; //set custom key ignition status= custom key in ignition

						REMOTE_START_msg_data[1]=RF_fob_number;//update the fob number
						if(engineRemoteStartRequest==3){ //first message let's close the doors
							REMOTE_START_msg_data[2]=0x16;//update the function request (1= lock ports) and requestor (6=remote_start)
							doorOpenTime=currentTime-1000; //refresh time, so that we will wait 2 more seconds

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
						//	doorOpenTime=currentTime+5000; //set the trigger in order to enter next 10 seconds
						//}

						//if(engineRemoteStartRequest==1){

						//	pressStartButton=1;
						//}
						onboardLed_blue_on();
						if (engineRemoteStartRequest>0) engineRemoteStartRequest--; //avoid to return here after the required messages were sent
					}
				}
			}
			*/
		}
	#endif


}
