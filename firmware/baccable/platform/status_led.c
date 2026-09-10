
// LED: Handles blinking of status light

#include "stm32f0xx_hal.h"
#include "platform/status_led.h"
#include "app/application_state.h"

static uint32_t led_blue_laston = 0;
static uint32_t led_red_laston = 0;
static uint32_t led_blue_lastoff = 0;
static uint32_t led_red_lastoff = 0;

/* Prepare the board's activity and fault indicators. */
void status_led_init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1; // the 2 leds
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;    // removed GPIO_MODE_OUTPUT_PP in order to let it work
                                                   // start&stop car command disabler
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // init mosfet too
    GPIO_InitStruct.Pin = Q10mosfet_Pin | Q11mosfet_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // it doesn't work anymore. we have to work on it if we need it

    // disabler 	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //     start&stop car functionality

    status_led_error_off();    // shut down red led
    status_led_activity_off(); // shut down blue led
}

/* Show a brief fault indication that remains distinguishable during heavy activity. */
void status_led_error(void) {
    // Make sure the LED has been off for at least LED_DURATION before turning on again
    // This prevents a solid status LED on a busy canbus
    if ((led_red_laston == 0) && ((HAL_GetTick() - led_red_lastoff) > LED_DURATION)) {
        HAL_GPIO_WritePin(LED_RED, led_light_on_bit);
        led_red_laston = HAL_GetTick();
    }
}

/* Turn off the board's fault indicator. */
void status_led_error_off(void) { HAL_GPIO_WritePin(LED_RED, !led_light_on_bit); }

/* Show a brief communication-activity indication. */
void status_led_activity(void) {
    // Make sure the LED has been off for at least LED_DURATION before turning on again
    // This prevents a solid status LED on a busy canbus
    if ((led_blue_laston == 0) && ((HAL_GetTick() - led_blue_lastoff) > LED_DURATION)) {
        HAL_GPIO_WritePin(LED_BLUE, led_light_on_bit);
        led_blue_laston = HAL_GetTick();
    }
}

/* Turn off the board's activity indicator. */
void status_led_activity_off(void) { HAL_GPIO_WritePin(LED_BLUE, !led_light_on_bit); }

/* End activity and fault flashes after their display interval. */
void status_led_process(void) {
    // If LED has been on for long enough, turn it off
    if ((led_blue_laston > 0) && ((currentTime - led_blue_laston) > LED_DURATION)) {
        HAL_GPIO_WritePin(LED_BLUE, !led_light_on_bit);
        led_blue_laston = 0;
        led_blue_lastoff = HAL_GetTick();
    }

    // If LED has been on for long enough, turn it off
    if ((led_red_laston > 0) && ((currentTime - led_red_laston) > LED_DURATION)) {
        HAL_GPIO_WritePin(LED_RED, !led_light_on_bit);
        led_red_laston = 0;
        led_red_lastoff = HAL_GetTick();
    }
}
