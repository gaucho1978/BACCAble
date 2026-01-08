
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_it.h"
#include "main.h"
/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_tim1_ch4_trig_com;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern UART_HandleTypeDef huart2;

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void){
	RCC->AHBENR |=RCC_AHBENR_GPIOAEN; //ensure clock is enabled on port gpioA

	uint8_t tmpBool01=0;
	while (1){
		for (volatile uint32_t i = 0; i < 10000000; i++){
			__asm("nop");
		}
		//now toggle leds without using HAL, to be more resilient
		if(tmpBool01){
			GPIOA->BSRR= GPIO_PIN_1 | GPIO_PIN_0 ; //set PA1 high (blue led) and set PA0 high (red led)

		}else{
			GPIOA->BSRR= (GPIO_PIN_1<<16) | (GPIO_PIN_0<<16) ; //set PA1 low (blue led) and set PA0 low (red led)
		}

		tmpBool01=!tmpBool01;
	}
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void){

	NVIC_SystemReset(); //reset the chip (temporary mask problems)
	//execution ends here

	RCC->AHBENR |=RCC_AHBENR_GPIOAEN; //ensure clock is enabled on port gpioA


	uint8_t tmpBool01=0;
	while (1){
		for (volatile uint32_t i = 0; i < 10000000; i++){
			__asm("nop");
		}
		//now toggle leds without using HAL, to be more resilient
		if(tmpBool01){
			GPIOA->BSRR= GPIO_PIN_1 | (GPIO_PIN_0 <<16); //set PA1 high (blue led) and set PA0 low (red led)

		}else{
			GPIOA->BSRR= (GPIO_PIN_1<<16) | GPIO_PIN_0 ; //set PA1 low (blue led) and set PA0 high (red led)
		}

		tmpBool01=!tmpBool01;
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel 4, 5, 6 and 7 interrupts.
  */
void DMA1_Channel4_5_6_7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_5_6_7_IRQn 0 */

  /* USER CODE END DMA1_Channel4_5_6_7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_tim1_ch4_trig_com);
  /* USER CODE BEGIN DMA1_Channel4_5_6_7_IRQn 1 */

  /* USER CODE END DMA1_Channel4_5_6_7_IRQn 1 */
}

void USB_IRQHandler(void)
{
  /* USER CODE BEGIN USB_IRQn 0 */

  /* USER CODE END USB_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_FS);
  /* USER CODE BEGIN USB_IRQn 1 */

  /* USER CODE END USB_IRQn 1 */
}

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}


