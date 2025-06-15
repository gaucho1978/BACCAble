/*
 * processingMessage0x00000226.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "processingExtendedMessage.h"

void processingExtendedMessage(){
	#if defined(C1baccable)
		if(immobilizerEnabled && (engineOnSinceMoreThan5seconds<500)){ //if immo enabled and engine is off
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
					floodTheBusStartTime=currentTime; //set initial time we started to flood the bus
					onboardLed_blue_on(); //light a led
				}

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

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C2cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C2cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();
				}
			}
			if (DynoStateMachine==3 && rx_msg_header.DLC>=4){ //we received a reply to dyno enable msg
				if(rx_msg_data[0]==0x03 && rx_msg_data[1]==0x6E && rx_msg_data[2]==0x30 && rx_msg_data[3]==0x02){ //if request was successful
					DynoModeEnabled=1;//success change complete

					DynoStateMachine=0xff; //disable state machine

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C2cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C2cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();
				}
			}

			if (DynoStateMachine!=0xff && rx_msg_header.DLC>=3){ //in any case
				if( rx_msg_data[1]==0x7F ){ //if request refused, abort all
					DynoStateMachine=0xff; //disable state machine

					//send message to master to inform about the status of Dyno
					uint8_t tmpArr2[2]={C1BusID,C2cmdDynoNotActive};
					if(DynoModeEnabled) tmpArr2[1]=C2cmdDynoActive;
					addToUARTSendQueue(tmpArr2, 2);

					onboardLed_blue_on();

				}
			}
			if(DynoStateMachine!=0xff){ //if we are running, send next message
				DYNO_msg_header.DLC=DYNO_msg_data[DynoStateMachine][0]+1;
				can_tx(&DYNO_msg_header, DYNO_msg_data[DynoStateMachine]); //add to the transmission queue
				onboardLed_blue_on();
				DynoStateMachineLastUpdateTime=currentTime;//save last time it was updated
			}
		}
	#endif


}
