#ifndef BACCABLE_STATE_SETTINGS_H
#define BACCABLE_STATE_SETTINGS_H
#include "app/runtime_config.h"
#include "stm32f0xx_hal.h"
typedef struct {
    uint8_t reserved;
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    int8_t pedal_map_power;
    uint8_t acc_virtual_pad_enabled;
    uint8_t smart_disable_start_stop_enabled;
    uint8_t is_diesel_enabled;
    uint8_t route_msg_enabled;
    uint8_t remote_start_enabled;
    uint8_t read_faults_enabled;
    uint8_t awd_disabler_enabled;
    uint8_t ipc_my23_is_installed;
    uint8_t regeneration_alert_enabled;
    uint8_t shift_indicator_enabled;
    uint16_t shift_threshold;
    uint8_t led_strip_controller_enabled;
    uint8_t dyno_mode_master_enabled;
    uint8_t front_brake_forcer_master;
    uint16_t launch_torque_threshold;
    uint8_t seatbelt_alarm_enabled;
    uint8_t disable_odometer_blink;
    uint8_t acc_autostart;
    uint8_t close_windows_with_door_lock;
    uint8_t open_windows_with_door_lock;
    uint8_t qv_exhaust_flap_function_enabled;
    uint8_t eujot_enabled;
    uint8_t parking_sensor_mute;
    uint8_t reverse_audio_mute;
    uint8_t rotate_readings;
    uint8_t usb_sniffer, usb_elm327;
#endif
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    uint8_t lights_animation_enabled;
#endif

    uint8_t esc_tc_customizator_enabled;

    uint8_t pedal_booster_enabled;

    uint8_t clear_faults_enabled;

    uint8_t show_race_mask;

    uint8_t park_mirror;

    uint8_t has_function_enabled;
} SettingsState;
extern SettingsState settings_state;
#endif
