/*
 * functions_BHbaccable.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#include "functions_BHbaccable.h"

#if defined(BHbaccable)

	void BHbaccableInitCheck(){
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_125K);//set can speed to 125kpbs
		can_enable(); //enable can port

		//prepare msg to send:
		//total frame number is on byte 0 from bit 7 to 3
		telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0xF8) | ((telematic_display_info_field_totalFrameNumber<<3) & 0xF8);
		//infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
		telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0x3F) | ((telematic_display_info_field_infoCode) & 0x3F);
		//I don't use UTF chars, but ascii, so bytes 2,4,6 can be set to zero
		telematic_display_info_msg_data[2]=0;
		telematic_display_info_msg_data[4]=0;
		telematic_display_info_msg_data[6]=0;
	}

	void BHperiodicCheck(){
		if(requestToSendOneFrame>0){ //if requested by a message received from master baccable
			//send one msg to write something on the dashboard each 50msec (one frame each 300msec)
			if (currentTime-lastSentTelematic_display_info_msg_Time>50){
				lastSentTelematic_display_info_msg_Time=currentTime;
				//prepare msg to send:
				//frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
				telematic_display_info_msg_data[0]=(telematic_display_info_msg_data[0] & ~0x07) | ((telematic_display_info_field_frameNumber>>2) & 0x07);
				telematic_display_info_msg_data[1]=(telematic_display_info_msg_data[1] & ~0xC0) | ((telematic_display_info_field_frameNumber<<6) & 0xC0);

				//UTF text 1 is on byte 2 and byte 3
				telematic_display_info_msg_data[3]=dashboardPageStringArray[paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char
				//UTF text 2 is on byte 4 (set to zero ) and byte 5
				telematic_display_info_msg_data[5]=dashboardPageStringArray[paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char
				//UTF text 3 is on byte 6 (set to zero) and byte 7
				telematic_display_info_msg_data[7]=dashboardPageStringArray[paramsStringCharIndex];
				paramsStringCharIndex++; //prepare to send next char
				//send it
				can_tx(&telematic_display_info_msg_header, telematic_display_info_msg_data); //transmit the packet

				telematic_display_info_field_frameNumber++; //prepare for next frame to send
				if( paramsStringCharIndex>=DASHBOARD_MESSAGE_MAX_LENGTH) { //if we sent the entire string
					paramsStringCharIndex=0; //prepare to send first char of the string
					telematic_display_info_field_frameNumber=0; //prepare to send first frame
					requestToSendOneFrame -= 1;
					onboardLed_blue_on();
				}
			}
		}
	}
#endif
