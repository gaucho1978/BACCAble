#include "uart.h"
#include "stm32f0xx_hal.h"
#include "main.h"
extern void Error_Handler(void);


uint8_t rxBuffer[UART_BUFFER_SIZE]; //buffer to receive the message from uart
uint8_t rxIndex = 0;
uint8_t syncObtained=0;

extern UART_HandleTypeDef huart2;

extern uint8_t dashboardPageStringArray[18];
extern uint8_t requestToSendOneFrame; //--// used with SHOW_PARAMS_ON_DASHBOARD define functionality //set to 1 to send one frame on dashboard

#if defined(SHOW_PARAMS_ON_DASHBOARD)
	uint8_t xxBusId=BhBusID;
#endif
#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
	uint8_t xxBusId=C1BusID;
#endif

#if (!defined(SHOW_PARAMS_ON_DASHBOARD) && !defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE))
	uint8_t xxBusId=0xFF; //impossible value, just to avoid compilation errors
#endif


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
    huart2.Init.BaudRate = 9600;
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
			if (rxBuffer[0] == xxBusId){ //if we received the expected begin byte
				if(syncObtained){
					#if defined(SHOW_PARAMS_ON_DASHBOARD)
						memcpy(&dashboardPageStringArray[0], &rxBuffer[1], 18); //copy array that we will use in the main
						if (requestToSendOneFrame<=2) requestToSendOneFrame +=1;//Send one frame
					#endif
					#if defined(SHOW_PARAMS_ON_DASHBOARD_MASTER_BACCABLE)
						//nothing to do for now
					#endif
					HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 19); //receive next frame
				}else{
					syncObtained=1;
					HAL_UART_Receive_IT(&huart2, &rxBuffer[1], 18); //receive remaining part of the frame
				}
			}else { //we did not receive the begin of the message. discard it
				syncObtained=0; //we lost sync
				HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1); //receive one char
			}
    }
}

// interrput called when message send is complete
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    if (huart->Instance == USART2){
        //message sent. what do we do?
    	onboardLed_blue_on(); //successfully sent
    }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	onboardLed_red_on();
}
