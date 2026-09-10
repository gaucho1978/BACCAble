#ifndef BACCABLE_STATE_COMFORT_H
#define BACCABLE_STATE_COMFORT_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    CAN_TxHeaderTypeDef acc_msg_header;
    uint8_t acc_msg_data[3];
    uint8_t new_wheel_pressed_button_id;
    uint8_t acc_was_engaged_when_res_was_pressed;
    CAN_TxHeaderTypeDef disable_start_and_stop_msg_header;
    uint8_t disable_start_and_stop_msg_data[8];
    uint8_t start_and_stop_enabled;
    uint8_t start_andstop_car_status;
    uint32_t last_time_start_andstop_disabler_button_pressed;
    uint8_t request_to_disable_start_and_stop;
    uint8_t cruise_control_disabled;
    uint8_t acc_disabled;
    uint8_t acc_engaged;
    uint8_t wheel_pressed_button_id;
    uint32_t last_pressed_speed_up_wheel_button_duration;
    uint32_t time_since_last_received_accelerator_message;
    uint8_t close_windows_request;
    uint32_t door_close_time;
    uint8_t door_locks_requests_counter;
    uint8_t open_windows_request;
    uint32_t door_open_time;
    uint8_t door_unlocks_requests_counter;
    uint8_t rf_requestor;
    uint8_t rf_fob_number;
    uint8_t force_q_vexhaust_valve_opened;
    uint32_t last_sent_q_vexhaust_valve_msg_time;
    CAN_TxHeaderTypeDef force_q_vexhaust_valve_msg_header[4];
    uint8_t force_q_vexhaust_valve_msg_data[4][8];
    uint8_t number_of_release_button_clicks;
    uint32_t releasebutton_first_click_time;
    uint32_t releasebutton_press_begin_time;
    uint32_t exhaust_valve_mosfet_command_time;
    uint8_t chinese_exhaust_valve_request;
    uint8_t chinese_valve_is_opened;
#endif
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    uint8_t lights_animation_state_machine;
#endif

    uint8_t has_button_press_requested;
} ComfortState;
extern ComfortState comfort_state;
#endif
