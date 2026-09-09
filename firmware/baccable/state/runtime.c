#include "state/runtime.h"
RuntimeState runtime_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .instruct_slave_boards_trigger_enabled = 0,
    .last_sent_autostart_msg = 0,
    .autostart_msg_counter = 0,
    .car_steady_counter = 0,
#endif

    .we_can_send_a_message_reply = 0,

    .low_consume_is_active = 0,

    .last_received_can_msg_time = 0,

    .all_processors_wakeup_time = 0,

    .usb_connected_to_slave = 0,
};
