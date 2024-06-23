#ifndef _LED_H
#define _LED_H

	#define LED_BLUE_Pin GPIO_PIN_1
	#define LED_BLUE_Port GPIOA
	#define LED_BLUE LED_BLUE_Port , LED_BLUE_Pin

	#define LED_RED_Pin GPIO_PIN_0
	#define LED_RED_Port GPIOA
	#define LED_RED LED_RED_Port , LED_RED_Pin

	//used only for start&stop car disabler functionality
	#define START_STOP_DISABLER_Pin GPIO_PIN_14
	#define START_STOP_DISABLER_Port GPIOA
	#define START_STOP_DISABLER START_STOP_DISABLER_Port , START_STOP_DISABLER_Pin


	#define LED_DURATION 25

	void onboardLed_init();
	void onboardLed_blue_blink(uint8_t numblinks);
	void onboardLed_red_blink(uint8_t numblinks);
	void onboardLed_red_on(void);
	void onboardLed_red_off(void);
	void onboardLed_blue_on(void);
	void onboardLed_blue_off(void);
	void onboardLed_process(void);

#endif
