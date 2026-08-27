// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"
#include "stm32f0xx_it.h"

int main(void){
	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	can_init(); //initialize can interface
	//onboardLed_red_on(); This line doesn't work cause hardware is still initiating
	#if defined(C1baccable)
		C1baccableInitCheck();
	#endif

	uart_init();


	#if (defined(BHbaccable) || defined(C2baccable))
		storage_init();
	#endif

	#if defined(ACT_AS_CANABLE) || defined(DEBUG_MODE) || defined(ENABLE_USB_MASS_STORAGE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
		//because of condensation usb could fail and chip remain stuck. the following line,
		// ensures USB init only if we are coming from first powering and not from a reset.
		if(RCC->CSR & RCC_CSR_PORRSTF){
			//onboardLed_red_blink(2);
			MX_USB_DEVICE_Init(); //if first poweron, init usb port.
		}
		RCC->CSR |= RCC_CSR_RMVF; //delete reset flags


	#endif

	#if (defined(C1baccable) || defined(C2baccable) )  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(BHbaccable) )
		BHbaccableInitCheck();
	#endif
		//HAL_Delay (100);

	while (1){

		currentTimeMainLoopDebug=currentTime;
		onboardLed_process();
		can_process();

		//elm327 function 26/08/2026 - while a host is really talking to the interpreter the inter chip serial
		//line belongs entirely to the diagnostic bridge: a message from the normal queue would collide with it
		//and put the link out of sync. Only skipped once a host is actually there, never while merely waiting.
		#if defined(C1baccable)
			if(!elm327_is_enabled())
		#endif
		processUART();

		#if defined(C1baccable) //elm327 function 26/08/2026
			elm327UsbStartIfRequested();		//usb bring up is done here, never from an interrupt
			elm327CheckActivationTimeout();		//switches the interpreter on once a host shows up, gives up if none does
			elm327_process();					//interprets one command per loop; does nothing while the mode is off
		#endif
		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable) //elm327 function 26/08/2026 - does nothing until C1 arms the bridge
			elmlink_process();
		#endif

		#if defined(C1baccable)
			#ifdef DEBUG_START_SNIFFER //sniffer function 24/08/2026 - bench test with no vehicle connected: can bus is silent+loopback (see can.c), so this injected message follows the very same rx/interception path as real traffic, just below in this loop
				if(debugSnifferAutoStarted==0 && currentTime>=20000){
					debugSnifferAutoStarted=1;
					if(snifferFunctionEnabled==0){
						snifferFunctionEnabled=1;
						elm327FunctionEnabled=0;
					snifferStart(); //same entry point used by the SNIFFER menu toggle
					//notify to C2 and BH the sniffer function status
					uint8_t tmpArrSniffer[2]={C2_Bh_BusID,C2_Bh_cmdSnifferEnabled};
					addToUARTSendQueue(tmpArrSniffer, 2);
					saveOnflash();
				}
				}

			#endif

			#ifdef DEBUG_START_ELM327
				if(debugElm327AutoStarted==0 && currentTime>=20000){
					debugElm327AutoStarted=1;
					if(elm327FunctionEnabled==0){
						elm327FunctionEnabled=1;
						snifferFunctionEnabled=0;
						elm327Start();
						saveOnflash();
					}
				}
			#endif
		#endif

		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
			snifferUsbStartIfRequested(); //sniffer function 24/08/2026 - usb re-enumeration is done here, never from an interrupt
			snifferUsbShutdownIfRequested(); //sniffer function 24/08/2026 - usb power down is done here too, for the same reason
			snifferCheckActivationTimeout(); //sniffer function 24/08/2026 - reverts to a plain baccable if the host never configures us within SNIFFER_ACTIVATION_TIMEOUT_MS

			#ifdef DEBUG_CAN_RX_SIMULATION //sniffer function 24/08/2026 - bench test with no vehicle connected: can bus is silent+loopback (see can.c), so this injected message follows the very same rx/interception path as real traffic, just below in this loop
				if(currentTime-debugSimulatedMsgLastInjectTime>=100){
					debugSimulatedMsgLastInjectTime=currentTime;
					CAN_TxHeaderTypeDef debugSimulatedMsgTxHeader;
					debugSimulatedMsgTxHeader.IDE=CAN_ID_STD;
					debugSimulatedMsgTxHeader.RTR=CAN_RTR_DATA;
					debugSimulatedMsgTxHeader.StdId=0x1EF;
					debugSimulatedMsgTxHeader.DLC=8;
					uint8_t debugSimulatedMsgTxData[8]={0,0,0,0,0,0,0,debugSimulatedMsgByteCounter};
					debugSimulatedMsgByteCounter++;
					can_tx(&debugSimulatedMsgTxHeader, debugSimulatedMsgTxData); //queued here, transmitted by can_process() on the next loop, looped back into the rx fifo
				}
			#endif
		#endif


		#if (defined(BHbaccable) || defined(C2baccable))
			//sniffer function 24/08/2026 - BEGIN
			//this used to pause huart2 (towards C1) while acting as usb pen drive, to avoid interfering with the file
			//transfer. disabled: STORAGE_Write_FS() always returns USBD_FAIL (writes are not implemented) and
			//STORAGE_Read_FS() only memcpy()s from flash, both trivially safe to interrupt on cortex-m0, so there is
			//no real interference to avoid with the current storage implementation. keeping huart2 paused here was
			//also a deadlock for the sniffer: pauseUart() disables the RX interrupt, so C2_Bh_cmdSnifferEnabled sent
			//by C1 while usbConnectedToSlave was already set could never be received to un-pause it.
			//if(currentTime>TIMING__C2_BH_USB_CONNECT_TO_C1_NOTIFICATION_DELAY_MS+300 && usbConnectedToSlave && snifferInUse==0){  //if we are using it as usb pen drive, stop serial line to avoid interferences
			//	pauseUart(&huart2); //stop serial line between chips
			//}
			//sniffer function 24/08/2026 - END
		#endif

		//elm327 function 26/08/2026 - feeds the interpreter with the bytes arrived from usb. Only while a host is
		//really there: with the mode off (or merely waiting) there is nothing on that port to read.
		#if defined(C1baccable)
			if(elm327_is_enabled()) cdc_process();
		#endif

		#if defined(ACT_AS_CANABLE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
			cdc_process(); //processa dati usb
			//#if defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
			//	processUART1(); //process schizzaForte serial messages
			//#endif
			//just for test
			//char *data = "Hello World from USB CDC\n";
			//CDC_Transmit_FS((uint8_t*)data, strlen(data));
			//HAL_Delay (1000);

			//just for test, we can periodically send a packet to can bus
			//we can set the can bus in loopback mode, to receive back each sent message.
			// To set loopback mode, in function can_enable (can.c) we shall set can_handle.Init.Mode = CAN_MODE_LOOPBACK
			// This way we will receive whatever we send
			// Before to send can messages, speed shall be set and can port shall be enabled,
			//can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
			//can_enable(); //enable can port
			//Then we can prepare and send the following test packet
			//CAN_TxHeaderTypeDef testMsgHeader;
			//testMsgHeader.IDE= CAN_ID_STD;
			//testMsgHeader.RTR = CAN_RTR_DATA;
			//testMsgHeader.StdId=0x0412;
			//testMsgHeader.DLC=5;
			//uint8_t testMsgData[8] = {0};
			//testMsgData[0]=0x01;
			//testMsgData[1]=0x01;
			//testMsgData[2]=0x01;
			//testMsgData[3]=0x01;
			//testMsgData[4]=0xE6; //pedal position
			//can_tx(&testMsgHeader, testMsgData);
		#endif

		#if defined(C1baccable)
			//elm327 function 26/08/2026 - with a host actually talking to the interpreter the baccable steps
			//aside completely: no UDS requests of its own on the bus, no traffic towards the other two chips
			//(the serial line serves the bridge), no dashboard refresh. elm327Stop() puts everything back.
			if(!elm327_is_enabled()){
			processUART1();
			C1baccablePeriodicCheck();
			} //elm327 function 26/08/2026

			//wake up each 14 seconds for 3,5sec+ tmpCounter , just for testing
/*
			if ((currentTime - lastTrigger) >= 14000) {
				lastTrigger = currentTime; // new window begin
				tmpCounter++;
			}

			if ((currentTime - lastTrigger) < tmpCounter) lastReceivedCanMsgTime = currentTime;

*/
			//lastReceivedCanMsgTime = currentTime; //always awake, just for test

			//just for test
			//currentDNAmode=0;
			//currentRpmSpeed= 1000;
			//if(currentDNAmode==0 && (currentRpmSpeed> 400) && neverSaved && currentTime>10000){ //engine on and drive style Natural
			//	neverSaved=0;
			//	playMotorJingle=255;
			//}

			/*
			lastReceivedCanMsgTime = currentTime; //always awake, just for test
			currentRpmSpeed=1000;
			if ((currentTime - lastTrigger) >= 10000) {
				lastTrigger = currentTime; // new window begin
				QV_exhaust_flap_function_enabled=1;
				if (chineseValveIsOpened==0){ //if valves are closed,
					ChineseExhaustValveRequest='O'; //open request
				}else{
					ChineseExhaustValveRequest='C'; //close request
				}
			}
			*/

		#endif

		#if defined(C2baccable)
			C2PeriodicCheck();
			//ESCandTCinversion=1;
		#endif

		//if(ESCandTCinversion==1) onboardLed_red_on(); //just for test

		#if defined(BHbaccable) //this is the baccable slave
			BHperiodicCheck();
		#endif

		#if (defined(C1baccable) || defined(C2baccable) || defined(BHbaccable))
			if(clearFaultsRequest>0){
				//clear faults if requested
				if(currentTime-last_sent_clear_faults_msg>25){
					last_sent_clear_faults_msg= currentTime;

					#if defined(C1baccable)
						if(clearFaultsRequest==255){
							//ask to Baccable on C2 and bH bus, to reset faults //to be done
							uint8_t tmpArr[1]={AllResetFaults};
							addToUARTSendQueue(tmpArr, 1);
						}
					#endif
					//send a reset request
					clearFaults_msg_header.ExtId= 0x18DA00F1 | ((uint32_t)clearFaultsRequest<<8);
					can_tx(&clearFaults_msg_header, clearFaults_msg_data); //transmit the request

					clearFaultsRequest--;
				}
			}
		#endif

		// If CAN message receive is pending, process the message
		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
			uint8_t snifferCanFramesRead=0; //sniffer function 24/08/2026 - counts the frames drained in this iteration
		#endif
		//elm327 function 27/08/2026 - with the interpreter running the frames are read inside elm327_process()
		//while it waits for the answer to a command: draining the fifo here would throw those answers away
		//before it ever sees them. The bridge slave does the same inside elmlink_process(). On C1
		//elm327_is_enabled() and elmlink_is_enabled() are always the same value (set together inside
		//elm327_set_enabled()), so the one flag covers the master and both slaves.
		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
			if(elmlink_is_enabled()) goto skipCanRxProcessing;
		#endif
		while( is_can_msg_pending(CAN_RX_FIFO0)){ //sniffer function 24/08/2026 - was an if: with the sniffer on we empty the whole rx fifo
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					onboardLed_blue_on();
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
				#endif

				#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
					if(snifferInUse) snifferPushFrame(&rx_msg_header, rx_msg_data); //sniffer function 24/08/2026 - raw copy to usb, normal decoding below is untouched
				#endif

				#if defined(C1baccable)
					lastReceivedCanMsgTime=currentTime;
				#endif

				if (rx_msg_header.RTR == CAN_RTR_DATA){
					switch(rx_msg_header.IDE){
						case CAN_ID_EXT:
							processingExtendedMessage();
							break;
						case CAN_ID_STD: //if standard ID
							processingStandardMessage();
							break;
						default:
					}
				}
			}
			#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
				snifferCanFramesRead++; //sniffer function 24/08/2026
				if(snifferInUse==0) break; //sniffer function 24/08/2026 - unchanged behaviour when the function is off: one frame per iteration
				if(snifferCanFramesRead>=SNIFFER_CAN_FRAMES_PER_LOOP) break; //sniffer function 24/08/2026 - bounded by the rx fifo depth, keeps the loop short
			#else
				break; //unchanged behaviour on ACT_AS_CANABLE: one frame per main loop iteration
			#endif
		}

		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
			skipCanRxProcessing: ; //elm327 function 26/08/2026 - the interpreter/bridge reads the fifo itself
		#endif

		#if defined(C1baccable) || defined(C2baccable) || defined(BHbaccable)
			if(snifferInUse) snifferFlush(); //sniffer function 24/08/2026 - non blocking, sends at most 64 bytes per iteration
		#endif

		//for debug, measure the loop duration
		if (currentTime-currentTimeMainLoopDebug>2){
			onboardLed_red_on();


		}
	}
}








