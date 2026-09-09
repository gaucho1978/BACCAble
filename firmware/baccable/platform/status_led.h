#ifndef BACCABLE_PLATFORM_STATUS_LED_H
#define BACCABLE_PLATFORM_STATUS_LED_H

#include "stm32f0xx_hal.h"

#define LED_BLUE_Pin GPIO_PIN_1
#define LED_BLUE_Port GPIOA
#define LED_BLUE LED_BLUE_Port, LED_BLUE_Pin

#define LED_RED_Pin GPIO_PIN_0
#define LED_RED_Port GPIOA
#define LED_RED LED_RED_Port, LED_RED_Pin

// used only for start&stop car disabler functionality
#define START_STOP_DISABLER_Pin GPIO_PIN_14
#define START_STOP_DISABLER_Port GPIOA
#define START_STOP_DISABLER START_STOP_DISABLER_Port, START_STOP_DISABLER_Pin

#define Q10mosfet_Pin GPIO_PIN_8 // PA8
#define Q10mosfet_Port GPIOA
#define Q10mosfet Q10mosfet_Port, Q10mosfet_Pin

#define Q11mosfet_Pin GPIO_PIN_7 // PA7
#define Q11mosfet_Port GPIOA
#define Q11mosfet Q11mosfet_Port, Q11mosfet_Pin

#define LED_DURATION 25

void status_led_init();
void status_led_blink_activity(uint8_t numblinks);
void status_led_blink_error(uint8_t numblinks);
void status_led_error(void);
void status_led_error_off(void);
void status_led_activity(void);
void status_led_activity_off(void);
void status_led_process(void);

#endif
