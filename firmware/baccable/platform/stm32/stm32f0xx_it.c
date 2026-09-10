
/* Includes ------------------------------------------------------------------*/
#include "platform/stm32/stm32f0xx_it.h"
/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_tim1_ch4_trig_com;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern UART_HandleTypeDef huart2;

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/* Halt operation and blink both status LEDs after a non-maskable fault. */
void NMI_Handler(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ensure clock is enabled on port gpioA

    uint8_t tmpBool01 = 0;
    while (1) {
        for (volatile uint32_t i = 0; i < 10000000; i++) {
            __asm("nop");
        }
        // now toggle leds without using HAL, to be more resilient
        if (tmpBool01) {
            GPIOA->BSRR = GPIO_PIN_1 | GPIO_PIN_0; // set PA1 high (blue led) and set PA0 high (red led)

        } else {
            GPIOA->BSRR =
                (GPIO_PIN_1 << 16) | (GPIO_PIN_0 << 16); // set PA1 low (blue led) and set PA0 low (red led)
        }

        tmpBool01 = !tmpBool01;
    }
}

/* Halt operation and alternate the status LEDs after a processor fault. */
void HardFault_Handler(void) {
    // execution ends here

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ensure clock is enabled on port gpioA

    uint8_t tmpBool01 = 0;
    while (1) {
        for (volatile uint32_t i = 0; i < 10000000; i++) {
            __asm("nop");
        }
        // now toggle leds without using HAL, to be more resilient
        if (tmpBool01) {
            GPIOA->BSRR = GPIO_PIN_1 | (GPIO_PIN_0 << 16); // set PA1 high (blue led) and set PA0 low (red
                                                           // led)

        } else {
            GPIOA->BSRR = (GPIO_PIN_1 << 16) | GPIO_PIN_0; // set PA1 low (blue led) and set PA0 high (red
                                                           // led)
        }

        tmpBool01 = !tmpBool01;
    }
}

/* Provide the required service-call entry point; this firmware has no service-call work. */
void SVC_Handler(void) {}

/* Provide the required deferred-service entry point; this firmware has no deferred-service work. */
void PendSV_Handler(void) {}

/* Advance the clock used by feature delays and timeouts. */
void SysTick_Handler(void) { HAL_IncTick(); }

/******************************************************************************/
/* STM32F0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f0xx.s).                    */
/******************************************************************************/

/* Handle completion and errors for the LED-strip transfer. */
void DMA1_Channel4_5_6_7_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_tim1_ch4_trig_com); }

/* Service USB connection and transfer events. */
void USB_IRQHandler(void) { HAL_PCD_IRQHandler(&hpcd_USB_FS); }

/* Service communication with the other BACCAble boards. */
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }

#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
/* Service communication with the pedal controller. */
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }
#endif
