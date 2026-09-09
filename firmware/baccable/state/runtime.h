#ifndef BACCABLE_STATE_RUNTIME_H
#define BACCABLE_STATE_RUNTIME_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    uint8_t instruct_slave_boards_trigger_enabled;
    uint32_t last_sent_autostart_msg;
    uint8_t autostart_msg_counter;
    uint8_t car_steady_counter;
#endif

    uint32_t we_can_send_a_message_reply;

    uint8_t uart_tx_msg[UART_BUFFER_SIZE];

    uint8_t low_consume_is_active;

    uint32_t last_received_can_msg_time;

    uint32_t all_processors_wakeup_time;

    uint8_t usb_inited;

    uint32_t last_uart_error_callback;

    uint8_t usb_connected_to_slave;
} RuntimeState;
extern RuntimeState runtime_state;
#endif
