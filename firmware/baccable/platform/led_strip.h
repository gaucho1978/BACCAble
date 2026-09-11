#ifndef BACCABLE_PLATFORM_LED_STRIP_H
#define BACCABLE_PLATFORM_LED_STRIP_H

#include "stm32f0xx_hal.h"

#include "math.h"
#include "platform/status_led.h"
#include "platform/system.h"

void led_strip_set_usb(uint8_t enabled);
void led_strip_init(void);
void led_strip_update(float volume, uint8_t colorPreset);
void Set_LED(uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue);
void Set_Brightness(uint8_t brightness);
void WS2812_Send(void);
void led_strip_shutdown();
void setItalianFlag(void);
void setGermanFlag(void);
void setEuropeanFlag(void);
void setLedsVumeter(uint8_t volume);
void MX_TIM1_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

void MX_DMA_Init(void);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void MX_GPIO_Init(void);
uint8_t myAbs(int8_t num);
#endif /* BACCABLE_PLATFORM_LED_STRIP_H */
