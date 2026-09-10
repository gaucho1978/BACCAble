#include "state/chassis.h"
ChassisState chassis_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .drive_train_control_module_reset_msg_header =
        {{.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA1AF1, .DLC = 3},
         {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA1AF1, .DLC = 7},
         {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA1AF1, .DLC = 3},
         {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA1AF1, .DLC = 3}},
    .drive_train_control_module_reset_msg_data = {{
                                                      0x02,
                                                      0x11,
                                                      0x01,
                                                  },
                                                  {
                                                      0x06,
                                                      0x2F,
                                                      0x2A,
                                                      0xAA,
                                                      0x03,
                                                      0x00,
                                                      0x00,
                                                  },
                                                  {
                                                      0x02,
                                                      0x3E,
                                                      0x80,
                                                  },
                                                  {
                                                      0x02,
                                                      0x10,
                                                      0x03,
                                                  }},
    .last_sent_drive_train_msg_time = 0,
    .brake_intervention_acc_esc_asr = 0,
#endif
#if defined(BACCABLE_C2)
    .dyno_mode_enabled = 0,
    .dyno_state_machine = 0xff,
    .dyno_msg_data = {{
                          0x02,
                          0x10,
                          0x03,
                      },
                      {
                          0x03,
                          0x22,
                          0x30,
                          0x02,
                      },
                      {0x05, 0x2E, 0x30, 0x02, 0x00, 0x01},
                      {0x05, 0x2E, 0x30, 0x02, 0xFF, 0x01},
                      {
                          0x02,
                          0x3E,
                          0x80,
                      }},
    .dyno_msg_header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA28F1, .DLC = 6},
    .last_sent_tester_presence_msg_time = 0,
    .dyno_state_machine_last_update_time = 0,
    .park_assist_button_press_count = 0,
    .last_sent_rear_brake_msg_time = 0,
    .rear_brake_msg_header = {{.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA28F1, .DLC = 5},
                              {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA28F1, .DLC = 8},
                              {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA28F1, .DLC = 3},
                              {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA28F1, .DLC = 3}},
    .rear_brake_msg_data = {{
                                0x04,
                                0x2F,
                                0x5A,
                                0xBD,
                                0x00,
                            },
                            {0x07, 0x2F, 0x5A, 0xBD, 0x03, 0x27, 0x10, 0x03},
                            {
                                0x02,
                                0x3E,
                                0x80,
                            },
                            {
                                0x02,
                                0x10,
                                0x40,
                            }},
#endif

    .lan_ebutton_press_begin_time = 0,

    .lan_ebutton_first_click_time = 0,

    .number_of_lane_button_clicks = 0,

    .awd_sequence = 0,

    .front_brake_forced = 0,

    .dyno_mode_enabled_on_master = 0,

    .launch_assist_enabled = 0,

    .stability_inverted = 0,
};
