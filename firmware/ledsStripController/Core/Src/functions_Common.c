/*
 * functions_Common.c
 *
 *  Created on: May 2, 2025
 *      Author: GauchoHP
 */

#include "functions_Common.h"

void floatToStr(char* str, float num, uint8_t precision, uint8_t maxLen) {
    uint8_t i = 0;

    // Gestione dei casi speciali NaN e Inf
    if (num != num) {  // NaN check
        if (maxLen > 3) {
            str[0] = 'N'; str[1] = 'a'; str[2] = 'N'; str[3] = '\0';
        }
        return;
    }
    if (num == (float)INFINITY) {
        if (maxLen > 3) {
            str[0] = 'I'; str[1] = 'n'; str[2] = 'f'; str[3] = '\0';
        }
        return;
    }
    if (num == (float)-INFINITY) {
        if (maxLen > 4) {
            str[0] = '-'; str[1] = 'I'; str[2] = 'n'; str[3] = 'f'; str[4] = '\0';
        }
        return;
    }

    // Calcolo del fattore di arrotondamento corretto
    float roundingFactor = 0.5f;
    for (uint8_t j = 0; j < precision; j++) {
        roundingFactor /= 10.0f;
    }
    if (num < 0) {
        num -= roundingFactor;  // Arrotondamento corretto per numeri negativi
    } else {
        num += roundingFactor;  // Arrotondamento per numeri positivi
    }

    // Gestione del segno
    if (num < 0) {
        if (i < maxLen - 1) {
            str[i++] = '-';
        }
        num = -num;
    }

    // Parte intera e parte decimale
    uint32_t intPart = (uint32_t)num;
    uint32_t scale = 1;
    for (uint8_t j = 0; j < precision; j++) {
        scale *= 10;
    }
    uint32_t decPart = (uint32_t)((num - intPart) * scale);

    // Conversione della parte intera
    uint8_t intStart = i;
    if (intPart == 0) {
        if (i < maxLen - 1) {
            str[i++] = '0';
        }
    } else {
        uint8_t count = 0;
        uint32_t tmp = intPart;
        while (tmp > 0) {
            tmp /= 10;
            count++;
        }
        for (uint8_t j = count; j > 0; j--) {
            if (i < maxLen - 1) {
                str[i + j - 1] = (intPart % 10) + '0';
            }
            intPart /= 10;
        }
        i += count;
    }

    // Conversione della parte decimale
    if (precision > 0 && i < maxLen - 1) {
        str[i++] = '.';
        for (uint8_t j = 0; j < precision; j++) {
            if (i < maxLen - 1) {
                decPart *= 10;
                str[i++] = (decPart / scale) + '0';
                decPart %= scale;
            }
        }
    }

    // Rimuovere zeri finali superflui
    if (precision > 0) {
        while (i > intStart && str[i - 1] == '0') {
            str[--i] = '\0';
        }
        if (i > intStart && str[i - 1] == '.') {
            str[--i] = '\0';
        }
    }

    // Aggiungere terminatore di stringa
    if (i < maxLen) {
        str[i] = '\0';
    } else if (maxLen > 0) {
        str[maxLen - 1] = '\0';
    }
}

uint8_t calculateCRC(uint8_t* data, uint8_t arraySize) {
	uint8_t crc = 0xFF;
	if(arraySize>1){
		//calculate sae_j1850 CRC-8 of the array (excluded last element, that will be used to store the final CRC
		for (uint8_t i=0;i<arraySize-1;i++){
			crc ^= data[i];
			for (int i = 0; i < 8; ++i){
				crc = (crc & 0x80) ? (crc << 1) ^ 0x1D : crc << 1;
			}
		}
		return (crc ^ 0xFF); //return calculated checksum
	}
	return 0; //nothing to calculate
}


//System Clock Configuration
void SystemClock_Config(void){
  HAL_Init();
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK){
    Error_Handler();
  }

  //the following part is used by usb, used by canable

  // Set USB clock source to HSI48 (48 MHz)
  RCC_PeriphCLKInitTypeDef PeriphClkInit= {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
	  Error_Handler();
  }
  __HAL_RCC_GPIOA_CLK_ENABLE();

}


void Error_Handler(void){
  __disable_irq();
  while (1){}
}

// Disable all interrupts
void system_irq_disable(void){
	__disable_irq();
	__DSB();
	__ISB();
}


// Enable all interrupts
void system_irq_enable(void){
        __enable_irq();
}

void system_hex32(char *out, uint32_t val){
	char *p = out + 8;
	*p-- = 0;
	while (p >= out) {
		uint8_t nybble = val & 0x0F;
		if (nybble < 10)
			*p = '0' + nybble;
		else
			*p = 'A' + nybble - 10;
		val >>= 4;
		p--;
	}
}
