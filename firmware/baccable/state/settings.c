#include "state/settings.h"
SettingsState settings_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .pedal_map_power = 0,
    .acc_virtual_pad_enabled = 0,
    .smart_disable_start_stop_enabled = 1,
    .is_diesel_enabled = 1,
    .route_msg_enabled = 0,
    .remote_start_enabled = 0,
    .read_faults_enabled = 1,
    .awd_disabler_enabled = 1,
    .ipc_my23_is_installed = 1,
    .regeneration_alert_enabled = 0,
    .shift_indicator_enabled = 0,
    #if defined(SHIFT_THRESHOLD)
    .shift_threshold = SHIFT_THRESHOLD,
    #endif
#endif
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    #if defined(SHIFT_THRESHOLD)
    #else
    .shift_threshold = 1500,
    #endif
#endif
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .led_strip_controller_enabled = 0,
    .dyno_mode_master_enabled = 1,
    .front_brake_forcer_master = 1,
    #if defined(LAUNCH_ASSIST_THRESHOLD)
    .launch_torque_threshold = LAUNCH_ASSIST_THRESHOLD,
    #endif
#endif
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    #if defined(LAUNCH_ASSIST_THRESHOLD)
    #else
    .launch_torque_threshold = 100,
    #endif
#endif
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .seatbelt_alarm_enabled = 0,
    .disable_odometer_blink = 0,
    .acc_autostart = 0,
    .close_windows_with_door_lock = 0,
    .open_windows_with_door_lock = 0,
    .qv_exhaust_flap_function_enabled = 0,
    .eujot_enabled = 0,
#endif
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    .lights_animation_enabled = 0,
#endif

    .esc_tc_customizator_enabled = 0,

    .pedal_booster_enabled = 0,

    .clear_faults_enabled = 1,

    .show_race_mask = 0,

    .park_mirror = 0,

    .has_function_enabled = 0,
};
