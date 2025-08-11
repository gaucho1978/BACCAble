#include "uart.h"
#include "stm32f0xx_hal.h"
#include "main.h"
#include "globalVariables.h"
extern void Error_Handler(void);

#define QUEUE_SIZE 10  // max queue size

typedef struct {
    uint8_t tx_buffer[QUEUE_SIZE][UART_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} SendQueue;

SendQueue queue_instance= {0};
SendQueue *tx_queue = &queue_instance;



uint32_t last_sent_serial_msg_time=0;
uint32_t lastMsgSentToC2Time=0;
uint32_t lastMsgSentToBHTime=0;

uint8_t rxBuffer[UART_BUFFER_SIZE]; //buffer to receive the message from uart
uint8_t rxIndex = 0;
uint8_t syncObtained=0;

//weCanSendAMessageReply identifies last time a message was received by BH and C2 baccable (used by BH and C2 baccable)
//requestToSendOneFrame is Set to 1 to send one frame on dashboard
//uartTxMsg array contains the serial message to send
//clearFaultsRequest, if enabled, sends  messages to clear faults

void uart_init(){

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure PA14 as Half-Duplex TX/RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;      // Open-Drain to support both TX and RX on the same pin
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Necessary to avoid floating states
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure USART2 in Half-Duplex mode
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 38400; //38400;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;  // Enable both TX and RX
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_HalfDuplex_Init(&huart2) != HAL_OK) {
    	//onboardLed_red_blink(15); //error
        Error_Handler();
    }

    // **Enable Half-Duplex mode manually by setting HDSEL bit in CR3**
    huart2.Instance->CR3 |= USART_CR3_HDSEL;

    // **Enable reception in interrupt mode**
    HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1);  // Start receiving one byte


    // **Enable the interrupt in the NVIC (if not already set in CubeMX)**
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    //onboardLed_blue_blink(10); //ok
}


//interrupt called when message is received
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
		// evaluate received message
    	if((rxBuffer[0]>=C1BusID) && (rxBuffer[0]<=C1_Bh_BusID)){ //if the received char indicates the beginning of a message
			if(syncObtained){ //if we were sync, we can process the message, since the first char is correct and the sync indicates that te remaining part too is complete
				#if defined(ACT_AS_CANABLE)
					onboardLed_blue_on();
				#endif

				switch(rxBuffer[0]){
					case C1BusID: //message directed to baccable connected to C1 bus

						#if defined(C1baccable)
							switch(rxBuffer[1]){
								case C2cmdForceFrontBrake:
									front_brake_forced=1; //update status
									launch_assist_enabled=1; //enable launch assist
									break;
								case C2cmdNormalFrontBrake:
									front_brake_forced=0;
									launch_assist_enabled=0; //disable launch assist
									break;
								case C2cmdDynoActive:
									DynoModeEnabledOnMaster=1; //dyno active
									break;
								case C2cmdDynoNotActive:
									DynoModeEnabledOnMaster=0; //dyno not active
									break;
								default:
									break;
							}
						#endif
						break;
					case C2BusID: //message directed to baccable connected to C2 bus
						#if (defined(C2baccable))
							switch (rxBuffer[1]){
								case C2cmdtoggleDyno: //dyno request
									if(front_brake_forced==0) dynoToggle();
									break;
								case C2cmdtoggleEscTc: // ESC/TC request
									//if we can, enable it
									if(!(DynoModeEnabled || DynoStateMachine!=0xff)) ESCandTCinversion=!ESCandTCinversion;

									if(ESCandTCinversion && function_show_race_mask){ //if enabled and race screen requested, notify C1 and BH
										uint8_t tmpArr1[2]={C1_Bh_BusID, C1BHcmdShowRaceScreen};
										addToUARTSendQueue(tmpArr1, 2);
									}else{
										uint8_t tmpArr1[2]={C1_Bh_BusID, C1BHcmdStopShowRaceScreen};
										addToUARTSendQueue(tmpArr1, 2);
									}

									break;
								case C2cmdForceFrontBrake: //force front brake
									front_brake_forced=5;
									break;
								case C2cmdNormalFrontBrake: //release the brake - we set it to 255, just to trigger serial msg sending in the main (sync) and not here (async), since async may cause concurrent variables write
									front_brake_forced=255;
									break;
								case C2cmdRaceMaskDefault: //do not request to show race mask
									function_show_race_mask=0;
									break;
								case C2cmdShowRaceMask: //request to show race mask
									function_show_race_mask=1;
									break;
								default:
							}


							//if(rxBuffer[1]==C2cmdGetStatus){};//nothing to do, since we just need to set weCanSendAMessageReply
							onboardLed_blue_on();
							weCanSendAMessageReply=HAL_GetTick();
						#endif

						break;
					case BhBusID: //message directed to baccable connected to BH bus
						#if defined(BHbaccable)
							switch(rxBuffer[1]){
								case BHcmdOdometerBlinkDisable: //odometer blink disable request
									disable_odometer_blink=1;
									break;
								case BHcmdOdometerBlinkDefault: //odometer blink default request
									disable_odometer_blink=0;
									break;
								case BHcmdFunctParkMirrorDisabled: //park mirror disable
									function_park_mirror=0;
									break;
								case BHcmdFunctParkMirrorEnabled: //park mirror enabled
									function_park_mirror=1;
									break;
								case BHcmdFunctParkMirrorStoreCurPos: //park mirror enabled and transfer current mirror position
									function_park_mirror=1;
									storeCurrentParkMirrorPosition=1;
									break;
								default:
							}
							weCanSendAMessageReply=HAL_GetTick();
						#endif
						break;
					case C2_Bh_BusID: //message directed to baccable connected to C2 and BH bus
						#if defined(C2baccable) || defined(BHbaccable)
							if(rxBuffer[1]==C2_Bh_cmdSetPedalBoostStatus){
								//third byte contains the pedal booster status
								function_pedal_booster_enabled=rxBuffer[2];
							}
							onboardLed_blue_on();
							#if defined(C2baccable)
								weCanSendAMessageReply=HAL_GetTick(); //we decided that only C2 can reply, otherwise errors may arise
							#endif
						#endif
						break;
					case BhBusIDparamString: //message directed to baccable connected to BH bus in order to transfer a parameter to print
							#if defined(BHbaccable)
								weCanSendAMessageReply=HAL_GetTick();
								memcpy(&dashboardPageStringArray[0], &rxBuffer[1], DASHBOARD_MESSAGE_MAX_LENGTH); //copy array that we will use in the main

								if (requestToSendOneFrame<=2) requestToSendOneFrame +=1;//Send one frame

							#endif
						break;
					case BhBusIDgetStatus:
						#if defined(BHbaccable)
							weCanSendAMessageReply=HAL_GetTick();
						#endif
						break;
					case BhBusChimeRequest:
						#if defined(BHbaccable)
							weCanSendAMessageReply=HAL_GetTick();
							requestToPlayChime=1;
						#endif
						break;
					case AllSleep: //message directed to all the modules, in order to request low consumption
						//Not used for now..
						break;
					case C2BusIDAllSleepAck: //message directed to baccable on C1 bus, generated by C2 baccable, to inform that he received the sleep request
						//if we are the baccable on C1 bus
						break;
					case BHBusIDAllSleepAck: //message directed to baccable on C1 bus, generated by BH baccable, to inform that he received the sleep request
						//if we are the baccable on C1 bus
						//Not used for now..
						break;
					case AllResetFaults: //message received by baccable on BH and C2 bus. we shall reset all faults
						#if (defined(C2baccable) || defined(BHbaccable))
							clearFaultsRequest=255;
							onboardLed_blue_on();
							#if defined(C2baccable)
								weCanSendAMessageReply=HAL_GetTick(); //we decided that only C2 is allowed to reply
							#endif
						#endif
						break;
					case C1_Bh_BusID: //message received by C1 and BH baccable.
						#if (defined(C1baccable) || defined(BHbaccable))
						//#if defined(BHbaccable)
							if(rxBuffer[1]==C1BHcmdShowRaceScreen){
								ESCandTCinversion=1;
							}
							if(rxBuffer[1]==C1BHcmdStopShowRaceScreen){
								ESCandTCinversion=0;
							}
						#endif
						break;
					default:
						//not expected to end up here
						break;
				}

				HAL_UART_Receive_IT(&huart2, &rxBuffer[0], UART_BUFFER_SIZE); //receive next frame
			}else{ //otherwise we were not sync, therefore we need to receive the remaining part of the message
				syncObtained=1;
				HAL_UART_Receive_IT(&huart2, &rxBuffer[1], UART_BUFFER_SIZE-1); //receive remaining part of the frame
			}
    	}else { //we did not receive the begin of the message. discard it
			syncObtained=0; //we lost sync
			//onboardLed_blue_on();
			HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1); //receive one char
		}
    }
}

// interrupt called when message send is complete
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    if (huart->Instance == USART2){
        //message sent. what do we do?
    	onboardLed_blue_on(); //successfully sent
    }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	onboardLed_red_on();
	//onboardLed_red_blink(2);
	if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
		// Overrun error (buffer pieno, dati persi)
		//onboardLed_blue_on();
	}
	if (huart->ErrorCode & HAL_UART_ERROR_FE) {
		// Framing error
		//onboardLed_blue_on();
	}
	if (huart->ErrorCode & HAL_UART_ERROR_NE) {
		// Noise detected

	}
	if (huart->ErrorCode & HAL_UART_ERROR_PE) {
		// Parity error

	}

	HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1);

}
/*
void enter_standby_mode(void) {
	onboardLed_blue_on();
    // Abilita SLEEPDEEP nel registro SCR
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // Abilita la modalità Standby
    PWR->CR |= PWR_CR_PDDS;

    // Abilita il wake-up pin (opzionale)
    //PWR->CSR |= PWR_CSR_EWUP;

    // Entra in Standby Mode
    __WFI();
}

void resetOtherProcessorsSleepStatus(){
	baccableC2_IsSleeping=0;
	baccableBH_IsSleeping=0;
}

uint8_t getOtherProcessorsSleepingStatus(){
	return (baccableC2_IsSleeping && baccableBH_IsSleeping);
}
*/


void addToUARTSendQueue(const uint8_t *data, size_t length) {

	if (tx_queue->count < QUEUE_SIZE) {  // Controlla se la coda è piena
		memset(tx_queue->tx_buffer[tx_queue->tail], 0x20, UART_BUFFER_SIZE);
		if (length>UART_BUFFER_SIZE) length= UART_BUFFER_SIZE;
		memcpy(tx_queue->tx_buffer[tx_queue->tail], data, length);
		tx_queue->tail = (tx_queue->tail + 1) % QUEUE_SIZE;
		tx_queue->count++;
	} else {
		// queue full, ignore the request
	}

}






void processUART() {
    if (tx_queue->count == 0) {
        // queue empty
        return;
    }

    //if we are C2 or BH baccable, we can reply only if we received a message directed to us few milliseconds ago
	#if (defined(C2baccable) || defined(BHbaccable))
		if(HAL_GetTick()-weCanSendAMessageReply<200){ //if less than 200msec has passed since last message received from master baccable, send a message
			if( __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) ){ //&& (huart2.State == HAL_UART_STATE_READY)
				if (HAL_UART_Transmit_IT(&huart2, tx_queue->tx_buffer[tx_queue->head], UART_BUFFER_SIZE) == HAL_OK){
					// update head index in a circular way
					tx_queue->head = (tx_queue->head + 1) % QUEUE_SIZE;
					tx_queue->count--;
				}else{
					onboardLed_red_on();
				}
			}
		}
	#endif

    //if we are C1 baccable, we can send a message each 250msec
	#if defined(C1baccable)
    	if(HAL_GetTick()-last_sent_serial_msg_time>250){ //each 250msec send a message (so that we left the time to receiver to reply)
			last_sent_serial_msg_time=HAL_GetTick();
			if( __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) ){ //&& (huart2.State == HAL_UART_STATE_READY)
				if (HAL_UART_Transmit_IT(&huart2, tx_queue->tx_buffer[tx_queue->head], UART_BUFFER_SIZE) == HAL_OK){
					//save time
					switch(tx_queue->tx_buffer[tx_queue->head][0]){
						case C2BusID:
						case C2_Bh_BusID: //only C2 is allowed to reply when this msg is received
						case AllResetFaults: //only C2 is allowed to reply when this msg is received
							lastMsgSentToC2Time=HAL_GetTick();
							break;
						case BhBusIDparamString:
						case BhBusIDgetStatus:
						case BhBusChimeRequest:
							lastMsgSentToBHTime=HAL_GetTick();
							break;
						default:
					}



					// update head index in a circular way
					tx_queue->head = (tx_queue->head + 1) % QUEUE_SIZE;
					tx_queue->count--;
				}else{
					onboardLed_red_on();
				}
			}
		}


    	if(HAL_GetTick()-lastMsgSentToC2Time>1000){
			lastMsgSentToC2Time=HAL_GetTick();
			//get status from C2
			//send request thu serial line

			//to correct after solving uart framing error
			uint8_t tmpArr1[2]={C2BusID,C2cmdGetStatus};
			addToUARTSendQueue(tmpArr1, 2); //commented for test
		}
		if(HAL_GetTick()-lastMsgSentToBHTime>1000){
			lastMsgSentToBHTime=HAL_GetTick();
			//get status from BH
			//send request thu serial line
			//to correct after solving uart framing error
			uint8_t tmpArr2[1]={BhBusIDgetStatus};
			addToUARTSendQueue(tmpArr2, 1);
		}

	#endif
}
