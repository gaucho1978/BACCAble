#include "app/application_state.h"

const char *FW_VERSION = _FW_VERSION;
#if defined(UCAN_BOARD_LED_INVERSION)
const uint8_t led_light_on_bit = 1;
#else
const uint8_t led_light_on_bit = 0;
#endif
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
UART_HandleTypeDef huart1;
#endif

UART_HandleTypeDef huart2;
