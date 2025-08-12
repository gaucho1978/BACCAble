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

		//load stored params
		leftParkMirrorHorizontalPos=	(uint8_t)readFromFlashBH(1);
		leftParkMirrorVerticalPos=		(uint8_t)readFromFlashBH(2);
		rightParkMirrorHorizontalPos=	(uint8_t)readFromFlashBH(3);
		rightParkMirrorVerticalPos=		(uint8_t)readFromFlashBH(4);

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

		if(function_park_mirror && (currentGear==0x0E)){ //if function parkmirror is enabled and reverse gear is seleected
			if(!restoreMirrorsPosition){ //if we are not returning to operative position
				switch(turnIndicator){
					case 0x02: //left arrow inserted
						if(!leftParkMirrorPositionRequired && !rightParkMirrorPositionRequired) storeCurrentMirrorPosition=1;//store current mirror position, if mirror was not previously lowered
						leftParkMirrorPositionRequired=1; //Enable sending command to move mirror
						break;
					case 0x01: //right arrow inserted
						if(!leftParkMirrorPositionRequired && !rightParkMirrorPositionRequired) storeCurrentMirrorPosition=1;//store current mirror position, if mirror was not previously lowered
						rightParkMirrorPositionRequired=1; //Enable sending command to move mirror
						break;
					default:
				}
			}
		}

		//Prepare msg to send: set Operative position of the mirrors
		parkMirrorMsgData[0]= leftMirrorHorizontalPos;
		parkMirrorMsgData[1]= leftMirrorVerticalPos;
		parkMirrorMsgData[2]= rightMirrorHorizontalPos;
		parkMirrorMsgData[3]= rightMirrorVerticalPos;

		//Prepare msg to send: if required, set park position of the mirrors
		if(!restoreMirrorsPosition){
			if(leftParkMirrorPositionRequired){
				//prepare message to send
				parkMirrorMsgData[0]= leftParkMirrorHorizontalPos;
				parkMirrorMsgData[1]= leftParkMirrorVerticalPos;
			}
			if(rightParkMirrorPositionRequired){
				//prepare message to send
				parkMirrorMsgData[2]= rightParkMirrorHorizontalPos;
				parkMirrorMsgData[3]= rightParkMirrorVerticalPos;
			}
		}
		if(restoreMirrorsPosition){ //if we have to restore operative side mirrors position
			if(currentTime-lastParkMirrorMsgTime>1000){ //each 1000msec send a packet
					can_tx(&parkMirrorMsgHeader, parkMirrorMsgData); //send msg
					lastParkMirrorMsgTime=currentTime;
			}
			if(currentTime-restoreMirrorsPositionRequestTime>6000){ //after 6 seconds
				restoreMirrorsPosition=0;
			}
		}else{


			if(leftParkMirrorPositionRequired || rightParkMirrorPositionRequired){ //if mirror movement is requested
				if(!storeCurrentMirrorPosition && currentGear==0x0E){ //if current pos was stored and gear is reversed
					if(currentTime-lastParkMirrorMsgTime>1000){ //each 1000msec send a packet
						can_tx(&parkMirrorMsgHeader, parkMirrorMsgData); //send msg
						lastParkMirrorMsgTime=currentTime;
					}
				}
				if(currentGear!=0x0E && parkMirrorsSteady){ //if reverse gear no more inserted and mirrors are not moving
					leftParkMirrorPositionRequired=0; //Disable sending command to move mirror
					rightParkMirrorPositionRequired=0;//Disable sending command to move mirror
					restoreMirrorsPosition=1; //request to restore mirrors to their original position
					restoreMirrorsPositionRequestTime=currentTime;
				}
			}
		}
	}

	uint8_t saveOnFlashBH(){
		//last page to store on flash is 0x0801 F800 (we can store 2 bytes each time)
		// and we shall erase entire page before write. one page size is FLASH_PAGE_SIZE (2048 bytes in st32F072)
		HAL_FLASH_Unlock(); //unlock flash

		//erase flash
		FLASH_EraseInitTypeDef eraseInitStruct;
		uint32_t pageError=0;
		eraseInitStruct.TypeErase= FLASH_TYPEERASE_PAGES;
		eraseInitStruct.PageAddress=LAST_PAGE_ADDRESS; //last page address begin
		eraseInitStruct.NbPages=1;
		if(HAL_FLASHEx_Erase(&eraseInitStruct,&pageError)!=HAL_OK){ //error during erase
			HAL_FLASH_Lock();
			onboardLed_red_blink(8);
			return 254; //error
		}

		//it seems that stm32F072 supports only writing 2byte words
		//write parameter
		uint8_t paramsNumber=4;
		uint16_t params[40] = {
				leftParkMirrorHorizontalPos,
				leftParkMirrorVerticalPos,
				rightParkMirrorHorizontalPos,
				rightParkMirrorVerticalPos,
		};

		for (uint8_t i = 0; i < paramsNumber; i++) {
		    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, LAST_PAGE_ADDRESS + (i * 4), params[i]) != HAL_OK) {
		        HAL_FLASH_Lock();
		        onboardLed_red_blink(9);
		        return 255;
		    }
		}

		//lock the flash
		HAL_FLASH_Lock();
		return 0;

	}

	uint16_t readFromFlashBH(uint8_t paramId){
		if(paramId<1) return 0;
		uint16_t tmpParam=*(volatile uint16_t*)(LAST_PAGE_ADDRESS+((paramId-1)*4));
		return tmpParam;
	}

#endif
