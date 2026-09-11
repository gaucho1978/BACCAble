#include "platform/system.h"
#include "platform/debug.h"

/* Prepare the clocks required for reliable device and USB operation. */
void SystemClock_Config(void) {
    HAL_Init();
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
#ifdef HSE_ENABLED_FOR_UCAN
    RCC_OscInitStruct.OscillatorType |= RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
#endif
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler(2000);
    }

    // configure CRS to stabilize HSI48

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
#ifdef HSE_ENABLED_FOR_UCAN
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
#endif
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler(1500);
    }

    // the following part is used by usb, used by canable

    // Set USB clock source to HSI48 (48 MHz)
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
#ifdef HSE_ENABLED_FOR_UCAN
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLCLK;
#endif

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler(1000);
    }
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* Stop normal operation and show a persistent fault indication. */
void Error_Handler(uint16_t halfPeriod) {

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ensure clock is enabled on port gpioA

    uint8_t tmpBool01 = 0;
    while (1) {

        // now toggle leds without using HAL, to be more resilient
        if (tmpBool01) {
            GPIOA->BSRR = GPIO_PIN_0 << 16; // set PA0 low (red led)

        } else {
            GPIOA->BSRR = GPIO_PIN_0; // set PA0 high (red led)
        }

        tmpBool01 = !tmpBool01;

        for (volatile uint32_t i = 0; i < (12500 * halfPeriod); i++) { // 12500cycles=1msec
            __asm("nop");
        }
    }
}

// Disable all interrupts
// Enable all interrupts
