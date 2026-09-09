#include "state/diagnostics.h"
DiagnosticsState diagnostics_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .uds_parameter_request_msg_header = {.IDE = CAN_ID_EXT,
                                         .RTR = CAN_RTR_DATA,
                                         .ExtId = 0x18DA10F1,
                                         .DLC = 3},
    .last_sent_uds_parameter_request_time = 0,
    .route_std_id_msg = 0xff,
    .route_offset = 0,
    .route_msg_id = 0,
    .route_msg_header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DAF1BA, .DLC = 8},
    .route_msg_data = {0x07, 0x62, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE},
    .seatbelt_alarm_disabled = 0xff,
    .seatbelt_alarm_status_request_time = 0,
    .seat_belt_msg_header = {{.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA60F1, .DLC = 6},
                             {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA60F1, .DLC = 3}},
    .seatbelt_msg_data = {{
                              0x05,
                              0x2F,
                              0x55,
                              0xA0,
                              0x03,
                              0x00,
                          },
                          {
                              0x02,
                              0x10,
                              0x03,
                          }},
#endif

    .clear_faults_request = 0,

    .last_sent_clear_faults_msg = 0,

    .clear_faults_msg_data = {0x04, 0x14, 0xff, 0xff, 0xff},

    .clear_faults_msg_header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DA00F1, .DLC = 5},
};
