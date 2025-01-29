//
// LED: Handles blinking of status light
//

#include "stm32f0xx_hal.h"
#include "onboardLed.h"


// Private variables
static uint32_t led_blue_laston = 0;
static uint32_t led_red_laston = 0;
static uint32_t led_blue_lastoff = 0;
static uint32_t led_red_lastoff = 0;
extern uint8_t led_light_on_bit; //declared in main.c

// Initialize LED GPIOs
void onboardLed_init(){
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;  //the 2 leds
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // removed GPIO_MODE_OUTPUT_PP in order to let it work start&stop car command disabler
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_14  ; //PA14 is used to disable start &stop car functionality
    GPIO_InitStruct.Pull = GPIO_NOPULL; //removed GPIO_PULLUP to let it work start&stop car command disabler
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    onboardLed_red_off(); //shut down red led
    onboardLed_blue_off(); //shut down blue led
    HAL_GPIO_WritePin(START_STOP_DISABLER, 1); //outputs High signal at PA14, used to disable start&stop car functionality
}


// Turn red LED on
void onboardLed_red_on(void){
	// Make sure the LED has been off for at least LED_DURATION before turning on again
	// This prevents a solid status LED on a busy canbus
	if(led_red_laston == 0 && HAL_GetTick() - led_red_lastoff > LED_DURATION){
		HAL_GPIO_WritePin(LED_RED, led_light_on_bit);
		led_red_laston = HAL_GetTick();
	}
}


// Turn red LED off
void onboardLed_red_off(void){
	HAL_GPIO_WritePin(LED_RED, !led_light_on_bit);
}


// Blink blue LED (blocking)
void onboardLed_blue_blink(uint8_t numblinks){
	uint8_t i;
	for(i=0; i<numblinks; i++){
		HAL_GPIO_WritePin(LED_BLUE, led_light_on_bit);
		HAL_Delay(100);
		HAL_GPIO_WritePin(LED_BLUE, !led_light_on_bit);
		HAL_Delay(100);
	}
}

void onboardLed_red_blink(uint8_t numblinks){
	uint8_t i;
	for(i=0; i<numblinks; i++){
		HAL_GPIO_WritePin(LED_RED, led_light_on_bit);
		HAL_Delay(100);
		HAL_GPIO_WritePin(LED_RED, !led_light_on_bit);
		HAL_Delay(100);
	}
}


// Attempt to turn on status LED
void onboardLed_blue_on(void){
	// Make sure the LED has been off for at least LED_DURATION before turning on again
	// This prevents a solid status LED on a busy canbus
	if(led_blue_laston == 0 && HAL_GetTick() - led_blue_lastoff > LED_DURATION){
		HAL_GPIO_WritePin(LED_BLUE, led_light_on_bit);
		led_blue_laston = HAL_GetTick();
	}
}

// turn off status LED
void onboardLed_blue_off(void){
	HAL_GPIO_WritePin(LED_BLUE, !led_light_on_bit);
}

// Process time-based LED events
void onboardLed_process(void){
	// If LED has been on for long enough, turn it off
	if(led_blue_laston > 0 && HAL_GetTick() - led_blue_laston > LED_DURATION){
		HAL_GPIO_WritePin(LED_BLUE, !led_light_on_bit);
		led_blue_laston = 0;
		led_blue_lastoff = HAL_GetTick();
	}

	// If LED has been on for long enough, turn it off
	if(led_red_laston > 0 && HAL_GetTick() - led_red_laston > LED_DURATION){
		HAL_GPIO_WritePin(LED_RED, !led_light_on_bit);
		led_red_laston = 0;
		led_red_lastoff = HAL_GetTick();
	}
}
