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
