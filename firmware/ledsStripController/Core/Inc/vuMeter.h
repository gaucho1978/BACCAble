#ifndef INC_VUMETER_H_
	#define INC_VUMETER_H_

	#include "stm32f0xx_hal.h"
	#include "math.h"
	#include "onboardLed.h"

	void vuMeterInit(void);
	void vuMeterUpdate(float volume, uint8_t colorPreset );
	void Set_LED (uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue);
	void Set_Brightness (uint8_t brightness);
	void WS2812_Send (void);
	void shutdownLedsStrip();
	void setItalianFlag(void);
	void setGermanFlag(void);
	void setEuropeanFlag(void);
	void setLedsVumeter(uint8_t volume);
	void MX_TIM1_Init(void);
	void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

	void MX_DMA_Init(void);
	void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
	void MX_GPIO_Init(void);
	extern void Error_Handler(void);
	uint8_t myAbs(int8_t num);
#endif /* INC_VUMETER_H_ */
