// the definition of ACT_AS_CANABLE shall be placed in main.h
#include "main.h"

int main(void){
	SystemClock_Config(); //set system clocks
	onboardLed_init(); //initialize onboard leds for debug purposes
	can_init(); //initialize can interface
	//onboardLed_red_on(); This line doesn't work cause hardware is still initiating
	#if defined(C1baccable)
		C1baccableInitCheck();
	#endif

	uart_init();

	#if defined(ACT_AS_CANABLE)
		MX_USB_DEVICE_Init();
	#endif

	#if (defined(C1baccable) || defined(C2baccable) )  //if required, let's automatically open the can bus
		//let's open the can bus because we may need data
		can_set_bitrate(CAN_BITRATE_500K);//set can speed to 500kpbs
		can_enable(); //enable can port
	#endif

	#if (defined(BHbaccable))
		BHbaccableInitCheck();
	#endif

	while (1){

		currentTime=HAL_GetTick();
		onboardLed_process();
		can_process();
		processUART();


		#if defined(ACT_AS_CANABLE)
			cdc_process(); //processa dati usb
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
			C1baccablePeriodicCheck();
		#endif

		#if defined(C2baccable)
			C2PeriodicCheck();
		#endif

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
		if(is_can_msg_pending(CAN_RX_FIFO0)){
			// If message received from bus, parse the frame
			if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK){

				#if defined(ACT_AS_CANABLE)
					uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
					onboardLed_blue_on();
					if(msg_len){
						CDC_Transmit_FS(msg_buf, msg_len); //transmit data via usb
					}
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
		}

		//for debug, measure the loop duration
		if (HAL_GetTick()-currentTime>2){
			onboardLed_red_on();
		}
	}
}








