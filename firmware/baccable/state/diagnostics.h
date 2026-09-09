#ifndef BACCABLE_STATE_DIAGNOSTICS_H
#define BACCABLE_STATE_DIAGNOSTICS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    CAN_TxHeaderTypeDef uds_parameter_request_msg_header;
    uint8_t uds_parameter_request_msg_data[8];
    uint32_t last_sent_uds_parameter_request_time;
    uint8_t route_std_id_msg;
    uint8_t route_offset;
    uint32_t route_msg_id;
    CAN_TxHeaderTypeDef route_msg_header;
    uint8_t route_msg_data[8];
    uint8_t seatbelt_alarm_disabled;
    uint32_t seatbelt_alarm_status_request_time;
    CAN_TxHeaderTypeDef seat_belt_msg_header[2];
    uint8_t seatbelt_msg_data[2][8];
#endif

    uint8_t clear_faults_request;

    uint32_t last_sent_clear_faults_msg;

    uint8_t clear_faults_msg_data[5];

    CAN_TxHeaderTypeDef clear_faults_msg_header;
} DiagnosticsState;
extern DiagnosticsState diagnostics_state;
#endif
