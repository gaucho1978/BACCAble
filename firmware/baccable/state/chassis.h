#ifndef BACCABLE_STATE_CHASSIS_H
#define BACCABLE_STATE_CHASSIS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    CAN_TxHeaderTypeDef drive_train_control_module_reset_msg_header[4];
    uint8_t drive_train_control_module_reset_msg_data[4][8];
    uint32_t last_sent_drive_train_msg_time;
    uint8_t brake_intervention_acc_esc_asr;
#endif
#if defined(BACCABLE_C2)
    uint8_t dyno_mode_enabled;
    uint8_t dyno_state_machine;
    uint8_t dyno_msg_data[5][6];
    CAN_TxHeaderTypeDef dyno_msg_header;
    uint32_t last_sent_tester_presence_msg_time;
    uint32_t dyno_state_machine_last_update_time;
    uint8_t park_assist_button_press_count;
    uint32_t last_sent_rear_brake_msg_time;
    CAN_TxHeaderTypeDef rear_brake_msg_header[4];
    uint8_t rear_brake_msg_data[4][8];
#endif

    uint32_t lan_ebutton_press_begin_time;

    uint32_t lan_ebutton_first_click_time;

    uint8_t number_of_lane_button_clicks;

    uint8_t awd_sequence;

    uint8_t front_brake_forced;

    uint8_t dyno_mode_enabled_on_master;

    uint8_t launch_assist_enabled;

    uint8_t stability_inverted;
} ChassisState;
extern ChassisState chassis_state;
#endif
