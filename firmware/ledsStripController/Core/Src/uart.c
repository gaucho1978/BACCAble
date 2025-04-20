#include "uart.h"
#include "stm32f0xx_hal.h"
#include "main.h"
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

//uint8_t baccableC2_IsSleeping=0;
//uint8_t baccableBH_IsSleeping=0;

extern uint32_t weCanSendAMessageReply; //identifies last time a message was received by BH and C2 baccable (used by BH and C2 baccable)

extern uint8_t front_brake_forced;

extern UART_HandleTypeDef huart2;
#if defined(BHbaccable)
	//SHOW_PARAMS_ON_DASHBOARD
	extern uint8_t dashboardPageStringArray[DASHBOARD_MESSAGE_MAX_LENGTH];
	extern uint8_t requestToSendOneFrame; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard
	//extern uint8_t uartTxMsg[UART_BUFFER_SIZE];  //this variable contains the serial message to send
#endif

extern uint8_t clearFaultsRequest; //if enabled, sends  messages to clear faults

#if defined(C2baccable)
	//DYNO
	extern uint8_t DynoModeEnabled;
	extern uint8_t DynoStateMachine;

	//ESC_TC_CUSTOMIZATOR_ENABLED
	extern uint8_t ESCandTCinversion;
#endif

//#if defined(LOW_CONSUME)
//	extern uint8_t lowConsumeIsActive; //this variable comes from lowConsume.c
//#endif


void uart_init(){
	//#if defined(SHOW_PARAMS_ON_DASHBOARD)
	//	xxBusId=BhBusIDparamString;
	//#endif
	//#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	//	xxBusId=C1BusID;
	//#endif

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
    	if((rxBuffer[0]>=C1BusID) && (rxBuffer[0]<=BhBusIDgetStatus)){ //if the received char indicates the beginning of a message
			if(syncObtained){ //if we were sync, we can process the message, since the first char is correct and the sync indicates that te remaining part too is complete
				#if defined(ACT_AS_CANABLE)
					onboardLed_blue_on();
				#endif

				switch(rxBuffer[0]){
					case C1BusID: //message directed to baccable connected to C1 bus
						//not used up to today
						#if defined(C1baccable)
							switch(rxBuffer[1]){
								case C2cmdForceFrontBrake:
									front_brake_forced=1; //update status
									break;
								case C2cmdNormalFrontBrake:
									front_brake_forced=0;
									break;
								default:
									break;
							}
						#endif
						break;
					case C2BusID: //message directed to baccable connected to C2 bus
						//not used up to today
						#if (defined(C2baccable))
							if(rxBuffer[1]==C2cmdtoggleDyno){ //dyno request
								if(front_brake_forced==0) dynoToggle();
							}

							if(rxBuffer[1]==C2cmdtoggleEscTc){ // ESC/TC request
								//if we can, enable it
								if(!(DynoModeEnabled || DynoStateMachine!=0xff)) ESCandTCinversion=!ESCandTCinversion;
							}
							if(rxBuffer[1]==C2cmdForceFrontBrake){ front_brake_forced=5;}; //force front brake
							if(rxBuffer[1]==C2cmdNormalFrontBrake){ front_brake_forced=255;}; //release the brake - we set it to 255, just to trigger serial msg sending in the main (sync) and not here (async), since async may cause concurrent variables write
							//if(rxBuffer[1]==C2cmdGetStatus){};//nothing to do, since we just need to set weCanSendAMessageReply

							onboardLed_blue_on();
							weCanSendAMessageReply=HAL_GetTick();
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
					case AllSleep: //message directed to all the modules, in order to request low consumption
						//#if (defined(ESC_TC_CUSTOMIZATOR_ENABLED) || defined(DYNO_MODE) ) //if we are the baccable on C2 bus
						//	uartTxMsg[0]= C2BusIDAllSleepAck;//we will communicate that we received the sleep request
						//#endif

						//#if defined(SHOW_PARAMS_ON_DASHBOARD) //if we are the baccable on BH bus
						//	uartTxMsg[0]= BHBusIDAllSleepAck;//we will communicate that we received the sleep request
						//#endif

						//#if (defined(ESC_TC_CUSTOMIZATOR_ENABLED) || defined(DYNO_MODE) || defined(SHOW_PARAMS_ON_DASHBOARD) )
						//	if( __HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC)){
						//		if (HAL_UART_Transmit_IT(&huart2, uartTxMsg, UART_BUFFER_SIZE) != HAL_OK){ //send it asyncronously (with interrupt) over uart
						//			onboardLed_red_on();
						//			//Error_Handler(); //manage error in case of fail
						//		}
						//	}
						//	//goto sleep
						//	enter_standby_mode();
						//#endif

						break;
					case C2BusIDAllSleepAck: //message directed to baccable on C1 bus, generated by C2 baccable, to inform that he received the sleep request
						//if we are the baccable on C1 bus
						//#if (defined(SMART_DISABLE_START_STOP) || defined(DISABLE_START_STOP) || defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(ROUTE_MSG) || defined(ACC_VIRTUAL_PAD) )
						//	baccableC2_IsSleeping=1;
						//	#if defined(LOW_CONSUME)
						//		lowConsumeIsActive=getOtherProcessorsSleepingStatus();
						//	#endif
						//#endif

						break;
					case BHBusIDAllSleepAck: //message directed to baccable on C1 bus, generated by BH baccable, to inform that he received the sleep request
						//if we are the baccable on C1 bus
						//#if (defined(SMART_DISABLE_START_STOP) || defined(DISABLE_START_STOP) || defined(IMMOBILIZER_ENABLED) || defined(LED_STRIP_CONTROLLER_ENABLED) || defined(SHIFT_INDICATOR_ENABLED) || defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE) || defined(ROUTE_MSG) || defined(ACC_VIRTUAL_PAD) )
						//	baccableBH_IsSleeping=1;
						//	#if defined(LOW_CONSUME)
						//		lowConsumeIsActive=getOtherProcessorsSleepingStatus();
						//	#endif
						//#endif

						break;
					case AllResetFaults: //message received by baccable on BH and C2 bus. we shall reset all faults
						#if (defined(C2baccable) || defined(BHbaccable))
							clearFaultsRequest=255;
							onboardLed_blue_on();
						#endif
						break;
					default:
						//not exptected to end up here
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
					if(tx_queue->tx_buffer[tx_queue->head][0]== C2BusID){
						lastMsgSentToC2Time=HAL_GetTick();
					}
					if((tx_queue->tx_buffer[tx_queue->head][0]== BhBusIDparamString) || (tx_queue->tx_buffer[tx_queue->head][0])==BhBusIDgetStatus){
						lastMsgSentToBHTime=HAL_GetTick();
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
