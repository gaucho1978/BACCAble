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
		leftParkMirrorHorizontalPos		=(uint8_t)readFromFlashBH(1);
		leftParkMirrorVerticalPos		=(uint8_t)readFromFlashBH(2);
		rightParkMirrorHorizontalPos	=(uint8_t)readFromFlashBH(3);
		rightParkMirrorVerticalPos		=(uint8_t)readFromFlashBH(4);
		storeCurrentParkMirrorPosition	=(uint8_t)readFromFlashBH(5);
		leftMirrorHorizontalOperativePos=(uint8_t)readFromFlashBH(6);
		leftMirrorVerticalOperativePos	=(uint8_t)readFromFlashBH(7);
		rightMirrorHorizontalOperativePos=(uint8_t)readFromFlashBH(8);
		rightMirrorVerticalOperativePos =(uint8_t)readFromFlashBH(9);

	}

	//DNA memory 18/10/2026 - BEGIN
	//C1 asks a drive style (BHcmdDnaMemoryRestore) at the first engine start after the car slept. The DNA selector
	//(DSCM) sends 0x256 every ~250 ms: byte 0 = 0x40 up (A->N->D->R), 0x20 down, 0x00 not pressed. The presses are made
	//by sending the frame just received from the selector again, with byte 0 changed, right after it:
	// - short press (every step but D->R): one changed frame, then the next 0x256 is let through ("not pressed");
	// - long press (D->R, always the last step): every 0x256 is changed for DNA_RESTORE_LONG_PRESS_MS, then nothing
	//   (the following frames of the selector carry the release).
	//The whole sequence, from the current style to the wanted one, is worked out first and sent without checking the
	//style meanwhile. DNA_RESTORE_CHECK_DELAY_MS after its end the style is checked: if it is not the wanted one the
	//sequence is worked out again and sent once more, then nothing. Abort at once if the user moves the selector or the
	//engine stops. Afterwards nothing is sent until the next request: the user is free to change the drive style.
	static uint8_t dnaRestorePosition(uint8_t mode){ //0x46C code -> position on the selector, 0xFF unknown
		switch(mode){
			case 0x04: return 0; //A
			case 0x00: return 1; //N
			case 0x02: return 2; //D
			case 0x0C: return 3; //R
			default:   return 0xFF;
		}
	}

	//works out the presses from the current style to dnaRestoreTarget. 0 = nothing to do (or style unknown)
	static uint8_t dnaRestorePlan(void){
		uint8_t cur=dnaRestorePosition(currentDNAmode);
		if(cur==0xFF || cur==dnaRestoreTarget) return 0;
		dnaRestoreLong=0;
		if(dnaRestoreTarget>cur){
			dnaRestoreDir=0x40; //up
			if(dnaRestoreTarget==3){ //R: the last step D->R is the long press
				dnaRestoreShortLeft=(uint8_t)(2-cur);
				dnaRestoreLong=1;
			}else{
				dnaRestoreShortLeft=(uint8_t)(dnaRestoreTarget-cur);
			}
		}else{
			dnaRestoreDir=0x20; //down
			dnaRestoreShortLeft=(uint8_t)(cur-dnaRestoreTarget);
		}
		dnaRestoreSkipNext=0;
		dnaRestoreLongStart=0;
		SNIFFER_DEBUG2(0x2700, ((uint32_t)dnaRestorePass<<16)|((uint32_t)cur<<8)|dnaRestoreTarget, ((uint32_t)dnaRestoreDir<<16)|((uint32_t)dnaRestoreShortLeft<<8)|dnaRestoreLong); //DNA memory: sequence worked out. v1=byte2 pass, byte1 current position, byte0 wanted (0 A, 1 N, 2 D, 3 R), v2=byte2 direction (0x40 up, 0x20 down), byte1 short presses, byte0 1 if long press D->R
		return 1;
	}

	static void dnaRestoreStop(uint8_t reason){
		SNIFFER_DEBUG2(0x2703, reason, ((uint32_t)dnaRestoreState<<8)|dnaRestorePass); //DNA memory: stopped. v1=reason (1 not started: no fresh 0x46C/0x256 or engine off, 2 nothing to do, 3 user moved the selector, 4 engine stopped, 5 done: wanted style reached, 6 done: second sequence sent), v2=byte1 state, byte0 pass
		dnaRestoreState=0;
	}

	//called on every 0x256 received (main loop): sends the presses right after the frame of the selector
	void dnaRestoreOn256(void){
		if(dnaRestoreState!=1) return;
		if(rx_msg_data[0] & 0x60){ dnaRestoreStop(3); return; } //a real press of the user (our own frames are never received)
		if(currentRpmSpeed<=DNA_RESTORE_ENGINE_RUNNING_RPM){ dnaRestoreStop(4); return; }
		uint8_t press=0;
		if(dnaRestoreSkipNext){
			dnaRestoreSkipNext=0; //the selector's "not pressed" goes through
		}else if(dnaRestoreShortLeft>0){
			dnaRestoreShortLeft--;
			press=dnaRestoreDir;
			dnaRestoreSkipNext=1;
		}else if(dnaRestoreLong){
			if(dnaRestoreLongStart==0) dnaRestoreLongStart=currentTime;
			if(currentTime-dnaRestoreLongStart<DNA_RESTORE_LONG_PRESS_MS) press=0x40;
			else dnaRestoreLong=0; //long press over: the next frames of the selector carry the release
		}
		if(press){
			uint8_t d[8];
			memcpy(d, rx_msg_data, 8);
			d[0]=(uint8_t)((d[0] & ~0x60) | press);
			can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, d); //the frame of the selector again, pressed
			//can_process(); //sent as soon as possible after the original
			SNIFFER_DEBUG2(0x2701, press, ((uint32_t)dnaRestoreShortLeft<<8)|dnaRestoreLong); //DNA memory: press sent. v1=byte 0 sent (0x40 up, 0x20 down), v2=byte1 short presses left, byte0 1 if long press pending/running
		}
		if(dnaRestoreShortLeft==0 && !dnaRestoreLong && !dnaRestoreSkipNext){ //sequence complete (after a short press: when its release went through)
			dnaRestoreState=2;
			dnaRestoreEndTime=currentTime;
		}
	}

	//Called from BHperiodicCheck() on every main loop iteration. It does not send any frame by itself: it starts a
	//sequence (state 1), aborts it if the engine stops, and checks the result once a sequence is over (state 2).
	//The presses themselves are sent by dnaRestoreOn256(), right after each 0x256 received from the selector.
	//dnaRestoreState: 0=idle, nothing is sent | 1=sequence running (dnaRestoreOn256 sends the presses) |
	//                 2=sequence over, waiting DNA_RESTORE_CHECK_DELAY_MS before checking the drive style
	static void dnaRestoreProcess(void){
		//--- 1) a new request arrived from C1 (stored by the uart interrupt as a letter: 'A','N','D' or 'R')
		if(dnaRestoreRequest){ //new request from C1
			char letter=(char)dnaRestoreRequest;
			dnaRestoreRequest=0; //consumed: one request starts at most one restore (first sequence + one repetition)
			dnaRestoreTarget=(letter=='A')?0:(letter=='N')?1:(letter=='D')?2:3; //position on the selector, A=0 ... R=3
			dnaRestorePass=1; //first sequence
			dnaRestoreState=0; //a new request replaces any restore still in progress
			//start only if everything needed is really there:
			// - engine running (the drive style is set only with the engine on, as decided by C1);
			// - 0x46C received recently: the current drive style is known, so the sequence can be worked out;
			// - 0x256 received recently: the DNA selector is on this bus and keeps sending its frames, which are the
			//   trigger for every press (on cars with a different selector, as Ambrotek's, it never arrives).
			//The times are 0 until the first message of that kind has been received since power on.
			if(currentRpmSpeed<=DNA_RESTORE_ENGINE_RUNNING_RPM || dnaRestoreLast46CTime==0 || dnaRestoreLast256Time==0 ||
			   currentTime-dnaRestoreLast46CTime>DNA_RESTORE_INPUT_MAX_AGE_MS || currentTime-dnaRestoreLast256Time>DNA_RESTORE_INPUT_MAX_AGE_MS){
				dnaRestoreStop(1); //selector not on this bus, style unknown or engine off: nothing is sent
			}else if(dnaRestorePlan()){ //works out direction, number of short presses and the final long press (D->R)
				dnaRestoreState=1; //from now on dnaRestoreOn256() sends the presses
			}else{
				dnaRestoreStop(2); //already in the wanted drive style (or current style unknown): nothing to do
			}
		}

		//--- 2) the engine stopped while restoring: abort at once, no further frame is sent
		if(dnaRestoreState!=0 && currentRpmSpeed<=DNA_RESTORE_ENGINE_RUNNING_RPM){
			dnaRestoreStop(4);
			return;
		}

		//--- 3) a sequence is over: after DNA_RESTORE_CHECK_DELAY_MS (time for the BCM to update 0x46C) check the style
		if(dnaRestoreState==2 && currentTime-dnaRestoreEndTime>=DNA_RESTORE_CHECK_DELAY_MS){
			if(dnaRestorePosition(currentDNAmode)==dnaRestoreTarget){
				dnaRestoreStop(5); //wanted drive style reached: done
			}else if(dnaRestorePass==1){
				//first sequence did not reach it: work out the presses again, from the drive style the car is in now
				dnaRestorePass=2;
				if(dnaRestorePlan()) dnaRestoreState=1; //sent once more; after it the check only logs the result (5 or 6), nothing more is sent
				else dnaRestoreStop(2);
			}else{
				dnaRestoreStop(6); //second sequence over too: give up, nothing more is sent until the next request
			}
		}
	}
	//DNA memory 18/10/2026 - END

	void BHperiodicCheck(){
		dnaRestoreProcess(); //DNA memory 18/10/2026
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

		if(function_park_mirror){ //if function parkmirror is enabled

			if(currentGear==0x0E){ //if reverse gear is selected
				exitReverseTime = 0; //Reset hysteresis timer while in Reverse
				neutralGearEntryTime = 0; //we are not in Neutral

				switch(turnIndicator){
					case 0x02: //left arrow inserted
						//if left and right mirror were not requested to move
						if(!leftParkMirrorPositionRequired && !rightParkMirrorPositionRequired){
							//if we are returning to operative position
							if(restoreOperativeMirrorsPosition){
								//forget it
								restoreOperativeMirrorsPosition=0;
							}else{
								//store current mirror position, because mirror was not previously lowered
								storeOperativeMirrorPosition=1;
							}
						}
						if(!leftParkMirrorPositionRequired){
							leftParkMirrorPositionRequired=1; //Enable sending command to move mirror
							leftParkMirrorPositionRequiredRequestTime=currentTime;
						}
						break;
					case 0x01: //right arrow inserted
						//if left and right mirror were not requested to move
						if(!leftParkMirrorPositionRequired && !rightParkMirrorPositionRequired){
							//if we are returning to operative position
							if(restoreOperativeMirrorsPosition){
								//forget it
								restoreOperativeMirrorsPosition=0;
							}else{
								//store current mirror position, because mirror was not previously lowered
								storeOperativeMirrorPosition=1;
							}
						}
						if(!rightParkMirrorPositionRequired){
						rightParkMirrorPositionRequired=1; //Enable sending command to move mirror
							rightParkMirrorPositionRequiredRequestTime=currentTime;
						}
						break;
					default:
				}

			}else if(currentGear==0x00){ //Neutral
				if(neutralGearEntryTime==0) neutralGearEntryTime=currentTime; //remember when Neutral began

				//A brief pass through Neutral (a gear shift crossing N: R->D, D->N->D, ...) holds the
				//exit-reverse countdown at zero, exactly like Reverse does, so the shift transient never
				//starts nor restarts the 10 s return timer. Once Neutral has lasted longer than a shift
				//transient we do nothing and leave exitReverseTime as it is.
				if(currentTime-neutralGearEntryTime < TIMING__BH____NEUTRAL_GEAR_TRANSIENT_MS){
					exitReverseTime = 0;
				}

			}else{ //if Drive (D), Park (P) or any other gear is selected
				neutralGearEntryTime = 0; //we are not in Neutral

				if((leftParkMirrorPositionRequired || rightParkMirrorPositionRequired) && exitReverseTime == 0){ //if mirrors are potentially not in operative position,
					exitReverseTime = currentTime; //Capture the moment we switched to D

					//if P gear is selected or engine shutted off, instantly return to operative position
					//park mirror fix 28/08/2026 - was currentRpmSpeed>400, which is engine RUNNING (see the same
					//test used as "only if engine on" further down): leaving reverse while driving therefore always
					//took this instant return path, and the 10 second delay right below was never reachable.
					//currentTime+10001 is how the instant return is obtained: both operands are uint32_t, so the
					//currentTime-exitReverseTime below underflows to a huge value and fires on this same loop.
					if((currentGear==0x0D)||(currentRpmSpeed<=400)) exitReverseTime=currentTime+10001;
                }



				//Trigger return only after 10 seconds of constant driving forward (or instantly in case of P)
				if(exitReverseTime != 0 && (currentTime - exitReverseTime > 10000)){
					if(leftParkMirrorPositionRequired || rightParkMirrorPositionRequired){ //if mirrors are potentially not in operative position,
					  restoreOperativeMirrorsPosition=1; //request to restore mirrors to their original position
					  restoreOperativeMirrorsPositionRequestTime=currentTime;
					}
				    leftParkMirrorPositionRequired=0; //stop sending message to set Park position for mirrors
				    rightParkMirrorPositionRequired=0;//stop sending message to set Park position for mirrors
                    exitReverseTime = 0; //Reset timer after execution
				}
			}

			//Prepare msg to send: set Operative position of the mirrors
			parkMirrorMsgData[0]= leftMirrorHorizontalOperativePos;
			parkMirrorMsgData[1]= leftMirrorVerticalOperativePos;
			parkMirrorMsgData[2]= rightMirrorHorizontalOperativePos;
			parkMirrorMsgData[3]= rightMirrorVerticalOperativePos;

			//Prepare msg to send: if required, set park position of the mirrors
			if(leftParkMirrorPositionRequired){
				parkMirrorMsgData[0]= leftParkMirrorHorizontalPos;
				parkMirrorMsgData[1]= leftParkMirrorVerticalPos;
			}
			if(rightParkMirrorPositionRequired){
				parkMirrorMsgData[2]= rightParkMirrorHorizontalPos;
				parkMirrorMsgData[3]= rightParkMirrorVerticalPos;
			}


			if(leftParkMirrorPositionRequired || rightParkMirrorPositionRequired || restoreOperativeMirrorsPosition){ //if required
				if(!storeOperativeMirrorPosition){ //if operative position was stored
					if(currentTime-lastParkMirrorMsgTime>900){ //each 900msec send a packet
						// if restore mirror position arrived less than xxx seconds ago,avoid sending packets
						//   in order to have a pause between the 2 commands
						if(restoreOperativeMirrorsPosition){
							if(currentTime-restoreOperativeMirrorsPositionRequestTime<TIMING__BH____PAUSE_BETWEEN_PARK_MIRROR_COMMANDS){
								//don't do anything
							}else{
								can_tx(&parkMirrorMsgHeader, parkMirrorMsgData); //send msg
								lastParkMirrorMsgTime=currentTime;
							}
						}

						//if left or right mirror movement is required
						if(leftParkMirrorPositionRequired || rightParkMirrorPositionRequired){
							//if left mirror movement was  requested more than 2-3 seconds ago (ensures a pause between commands)
							if(currentTime-leftParkMirrorPositionRequiredRequestTime>=TIMING__BH____PAUSE_BETWEEN_PARK_MIRROR_COMMANDS){
								//if right mirror movement was  requested more than 2-3 seconds ago (ensures a pause between commands)
								if(currentTime-rightParkMirrorPositionRequiredRequestTime>=TIMING__BH____PAUSE_BETWEEN_PARK_MIRROR_COMMANDS){
									if(currentRpmSpeed>400 ){ //only if engine on, continue moving mirror
										//send a message
										can_tx(&parkMirrorMsgHeader, parkMirrorMsgData); //send msg
										lastParkMirrorMsgTime=currentTime;
									}
								}
							}
						}
					}
				}
				if(restoreOperativeMirrorsPosition){
					if(currentTime-restoreOperativeMirrorsPositionRequestTime>15000+TIMING__BH____PAUSE_BETWEEN_PARK_MIRROR_COMMANDS){ //after 15 seconds
						restoreOperativeMirrorsPosition=0;
					}
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
		uint8_t paramsNumber=9;
		uint16_t params[40] = {
				leftParkMirrorHorizontalPos,
				leftParkMirrorVerticalPos,
				rightParkMirrorHorizontalPos,
				rightParkMirrorVerticalPos,
				parkMirrorOperativePositionNotStored,
				leftMirrorHorizontalOperativePos,
				leftMirrorVerticalOperativePos,
				rightMirrorHorizontalOperativePos,
				rightMirrorVerticalOperativePos,

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
