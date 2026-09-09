#ifndef BACCABLE_VEHICLE_FRAME_HANDLERS_H
#define BACCABLE_VEHICLE_FRAME_HANDLERS_H
#include "stm32f0xx_hal.h"
void vehicle_handle_light_animation(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_adaptive_cruise(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_clock(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_park_assist(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_regeneration(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_chime(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_transmission_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_mirror_position(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_cruise_control(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_lane_button(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_brightness(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_chassis_aux(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_oil(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_start_stop(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_stability_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_body_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_battery_aux(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_battery(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_accelerator(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_body_gear(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_volume(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_odometer(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_body_lights(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_gear(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_radio_buttons(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_drive_style_display(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_suspension(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_transmission_temperature(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_clutch(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_gear_lever(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_vehicle_speed(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_engine_torque(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_brake_pedal(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
void vehicle_handle_display_content(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data);
#endif
