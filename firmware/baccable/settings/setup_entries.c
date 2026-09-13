/*
 * setup_entries.c
 *
 * Single source of truth for setup menu entries.
 *
 * To add a simple on/off option:
 *   1. Add the runtime field in state/settings.c and state/settings.h.
 *   2. Add one SETUP_TOGGLE(...) entry below with its menu text.
 *
 * Declare the shared entry type and numeric bounds below. Callbacks retain
 * feature-specific behavior; common rendering and draft editing live in setup_menu.c.
 */

#include "settings/setup_menu.h"
#include "features/menu.h"
#include "features/ibs_override.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"

    #define SETUP_FLASH_IMMOBILIZER 1
    #define SETUP_FLASH_START_STOP 2
    #define SETUP_FLASH_LED_CONTROLLER 3
    #define SETUP_FLASH_SHIFT_INDICATOR 4
    #define SETUP_FLASH_SHIFT_RPM 5
    #define SETUP_FLASH_MY23_IPC 6
    #define SETUP_FLASH_ROUTE_MESSAGES 7
    #define SETUP_FLASH_DYNO 8
    #define SETUP_FLASH_ACC_VIRTUAL_PAD 9
    #define SETUP_FLASH_BRAKES_OVERRIDE 10
    #define SETUP_FLASH_4WD_DISABLER 11
    #define SETUP_FLASH_REMOTE_START 12
    #define SETUP_FLASH_CLEAR_FAULTS 13
    #define SETUP_FLASH_ESC_TC_CUSTOMIZER 14
    #define SETUP_FLASH_READ_FAULTS 15
    #define SETUP_FLASH_DIESEL_PARAMS 16
    #define SETUP_FLASH_REGEN_ALERT 17
    #define SETUP_FLASH_LAUNCH_TORQUE 18
    #define SETUP_FLASH_SEATBELT_ALARM 19
    #define SETUP_FLASH_PEDAL_BOOSTER 20
    #define SETUP_FLASH_ODOMETER_BLINK 21
    #define SETUP_FLASH_SHOW_RACE_MASK 22
    #define SETUP_FLASH_PARK_MIRROR 23
    #define SETUP_FLASH_ACC_AUTOSTART 24
    #define SETUP_FLASH_CLOSE_WINDOWS 25
    #define SETUP_FLASH_OPEN_WINDOWS 26
    #define SETUP_FLASH_HAS_VIRTUAL_PAD 27
    #define SETUP_FLASH_QV_EXHAUST_FLAP 28
    #define SETUP_FLASH_PEDAL_POWER 29
    #define SETUP_FLASH_EUJOT 30
    #define SETUP_FLASH_PDC_MUTE 31
    #define SETUP_FLASH_REVERSE_AUDIO 32
    #define SETUP_FLASH_ROTATE 33

    #define SETUP_TEXT_HIDDEN 0

    #if defined(IMMOBILIZER_ENABLED)
        #define DEFAULT_IMMOBILIZER 1
    #else
        #define DEFAULT_IMMOBILIZER 0
    #endif

    #if defined(SMART_DISABLE_START_STOP)
        #define DEFAULT_START_STOP 1
    #else
        #define DEFAULT_START_STOP 0
    #endif

    #if defined(LED_STRIP_CONTROLLER_ENABLED)
        #define DEFAULT_LED_CONTROLLER 1
    #else
        #define DEFAULT_LED_CONTROLLER 0
    #endif

    #if defined(SHIFT_INDICATOR_ENABLED)
        #define DEFAULT_SHIFT_INDICATOR 1
    #else
        #define DEFAULT_SHIFT_INDICATOR 0
    #endif

    #if defined(SHIFT_THRESHOLD)
        #define DEFAULT_SHIFT_RPM SHIFT_THRESHOLD
    #else
        #define DEFAULT_SHIFT_RPM 3500
    #endif

    #if defined(IPC_MY23_IS_INSTALLED)
        #define DEFAULT_MY23_IPC 1
    #else
        #define DEFAULT_MY23_IPC 0
    #endif

    #if defined(ROUTE_MSG)
        #define DEFAULT_ROUTE_MESSAGES 1
    #else
        #define DEFAULT_ROUTE_MESSAGES 0
    #endif

    #if defined(DYNO_MODE_MASTER)
        #define DEFAULT_DYNO 1
    #else
        #define DEFAULT_DYNO 0
    #endif

    #if defined(ACC_VIRTUAL_PAD)
        #define DEFAULT_ACC_VIRTUAL_PAD 1
    #else
        #define DEFAULT_ACC_VIRTUAL_PAD 0
    #endif

    #if defined(FRONT_BRAKE_FORCER_MASTER)
        #define DEFAULT_BRAKES_OVERRIDE 1
    #else
        #define DEFAULT_BRAKES_OVERRIDE 0
    #endif

    #if defined(_4WD_DISABLER)
        #define DEFAULT_4WD_DISABLER 1
    #else
        #define DEFAULT_4WD_DISABLER 0
    #endif

    #if defined(REMOTE_START_ENABLED)
        #define DEFAULT_REMOTE_START 1
    #else
        #define DEFAULT_REMOTE_START 0
    #endif

    #if defined(CLEAR_FAULTS_ENABLED)
        #define DEFAULT_CLEAR_FAULTS 1
    #else
        #define DEFAULT_CLEAR_FAULTS 0
    #endif

    #if defined(ESC_TC_CUSTOMIZATOR_MASTER)
        #define DEFAULT_ESC_TC_CUSTOMIZER 1
    #else
        #define DEFAULT_ESC_TC_CUSTOMIZER 0
    #endif

    #if defined(READ_FAULTS_ENABLED)
        #define DEFAULT_READ_FAULTS 1
    #else
        #define DEFAULT_READ_FAULTS 0
    #endif

    #if defined(IS_DIESEL)
        #define DEFAULT_DIESEL_PARAMS 1
    #else
        #define DEFAULT_DIESEL_PARAMS 0
    #endif

    #if defined(REGENERATION_ALERT_ENABLED)
        #define DEFAULT_REGEN_ALERT 1
    #else
        #define DEFAULT_REGEN_ALERT 0
    #endif

    #if defined(LAUNCH_THRESHOLD)
        #define DEFAULT_LAUNCH_TORQUE LAUNCH_THRESHOLD
    #else
        #define DEFAULT_LAUNCH_TORQUE 100
    #endif

    #if defined(SEATBELT_ALARM_DISABLED)
        #define DEFAULT_SEATBELT_ALARM 0
    #else
        #define DEFAULT_SEATBELT_ALARM 1
    #endif

    #if defined(PEDAL_BOOSTER_ENABLED)
        #define DEFAULT_PEDAL_BOOSTER PEDAL_BOOSTER_ENABLED
    #else
        #define DEFAULT_PEDAL_BOOSTER 0
    #endif

    #if defined(DISABLE_ODOMETER_BLINK)
        #define DEFAULT_ODOMETER_BLINK 1
    #else
        #define DEFAULT_ODOMETER_BLINK 0
    #endif

    #if defined(SHOW_RACE_MASK)
        #define DEFAULT_SHOW_RACE_MASK 1
    #else
        #define DEFAULT_SHOW_RACE_MASK 0
    #endif

    #if defined(PARK_MIRROR)
        #define DEFAULT_PARK_MIRROR 1
    #else
        #define DEFAULT_PARK_MIRROR 0
    #endif

    #if defined(ACC_AUTOSTART)
        #define DEFAULT_ACC_AUTOSTART ACC_AUTOSTART
    #else
        #define DEFAULT_ACC_AUTOSTART 0
    #endif

    #if defined(CLOSE_WINDOWS)
        #define DEFAULT_CLOSE_WINDOWS CLOSE_WINDOWS
    #else
        #define DEFAULT_CLOSE_WINDOWS 0
    #endif

    #if defined(OPEN_WINDOWS)
        #define DEFAULT_OPEN_WINDOWS OPEN_WINDOWS
    #else
        #define DEFAULT_OPEN_WINDOWS 0
    #endif

    #if defined(HAS_VIRTUAL_PAD)
        #define DEFAULT_HAS_VIRTUAL_PAD 1
    #else
        #define DEFAULT_HAS_VIRTUAL_PAD 0
    #endif

    #if defined(QV_EXHAUST_FLAP_FUNCTION_ENABLED)
        #define DEFAULT_QV_EXHAUST_FLAP 1
    #else
        #define DEFAULT_QV_EXHAUST_FLAP 0
    #endif

    #if defined(PEDAL_MAP_POWER)
        #define DEFAULT_PEDAL_POWER ((uint16_t)(uint8_t)PEDAL_MAP_POWER)
    #else
        #define DEFAULT_PEDAL_POWER 0
    #endif

    #define DEFAULT_EUJOT 0

    #define SETUP_TOGGLE(flash, text, def, variable) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), text, 0, UI_ENTRY_TOGGLE, 0, 0, 0, 0}
    #define SETUP_TOGGLE_ACTION(flash, text, def, variable, action_fn) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), text, 0, UI_ENTRY_TOGGLE, 0, 0, 0, action_fn}
    #define SETUP_TOGGLE_RENDER_ACTION(flash, text, def, variable, render_fn, action_fn) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), text, render_fn, UI_ENTRY_ENUM, 0, 0, 0, action_fn}
    #define SETUP_VALUE8_ACTION(flash, text, max, def, variable, render_fn, action_fn) \
        {flash, max, def, SETUP_VALUE_UINT8, &(variable), text, render_fn, UI_ENTRY_ENUM, 0, 0, 0, action_fn}
    #define SETUP_NUMBER(flash, text, max, def, type, variable, render_fn, min, step, action_fn) \
        {flash, max, def, type, &(variable), text, render_fn, UI_ENTRY_NUMBER, min, (type == SETUP_VALUE_INT8_AS_UINT8 ? 10 : max), step, action_fn}
    #define SETUP_HIDDEN_TOGGLE(flash, def, variable) \
        {flash, 1, def, SETUP_VALUE_UINT8, &(variable), SETUP_TEXT_HIDDEN, 0, UI_ENTRY_TOGGLE, 0, 0, 0, 0}

static void setup_action_usb_mode(void);
static void setup_render_usb_mode(void);
static void setup_action_start_stop(void);
static void setup_action_esc_tc(void);
static void setup_action_diesel_params(void);
static void setup_action_odometer_blink(void);
static void setup_action_pedal_booster(void);
static void setup_action_pedal_power(void);
static void setup_action_acc_autostart(void);
static void setup_action_close_windows(void);
static void setup_action_open_windows(void);
static void setup_action_has_virtual_pad(void);

static void setup_render_launch_torque(void);
static void setup_render_shift_rpm(void);
static void setup_render_diesel_params(void);
static void setup_render_pedal_booster(void);
static void setup_render_pedal_power(void);
static void setup_render_acc_autostart(void);
static void setup_render_close_windows(void);
static void setup_render_open_windows(void);

// Common entry patterns:
//   SETUP_TOGGLE(SETUP_FLASH_XXX, "Menu Text", DEFAULT_XXX, variable)
//   SETUP_TOGGLE_ACTION(SETUP_FLASH_XXX, "Menu Text", DEFAULT_XXX, variable, setup_action_xxx)
//   SETUP_VALUE8_ACTION(SETUP_FLASH_XXX, "Menu Text", max, DEFAULT_XXX, variable, setup_render_xxx,
//   setup_action_xxx)
const SetupParam setup_params[] = {
    // Core setup
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_IMMOBILIZER, DEFAULT_IMMOBILIZER, security_state.immobilizer_enabled),
    SETUP_TOGGLE_ACTION(SETUP_FLASH_START_STOP, "Stop block", DEFAULT_START_STOP,
                        settings_state.smart_disable_start_stop_enabled, setup_action_start_stop),
    SETUP_NUMBER(SETUP_FLASH_LAUNCH_TORQUE, "Launch Nm", 600, DEFAULT_LAUNCH_TORQUE,
                 SETUP_VALUE_UINT16, settings_state.launch_torque_threshold, setup_render_launch_torque, 25, 25, 0),
    SETUP_TOGGLE(SETUP_FLASH_LED_CONTROLLER, "LED strip", DEFAULT_LED_CONTROLLER,
                 settings_state.led_strip_controller_enabled),
    SETUP_TOGGLE(SETUP_FLASH_SHIFT_INDICATOR, "Shift light", DEFAULT_SHIFT_INDICATOR,
                 settings_state.shift_indicator_enabled),
    SETUP_NUMBER(SETUP_FLASH_SHIFT_RPM, "Shift RPM", 6000, DEFAULT_SHIFT_RPM,
                 SETUP_VALUE_UINT16, settings_state.shift_threshold, setup_render_shift_rpm, 1500, 250, 0),
    SETUP_TOGGLE(SETUP_FLASH_MY23_IPC, "MY23 display", DEFAULT_MY23_IPC,
                 settings_state.ipc_my23_is_installed),
    SETUP_TOGGLE(SETUP_FLASH_REGEN_ALERT, "DPF alert", DEFAULT_REGEN_ALERT,
                 settings_state.regeneration_alert_enabled),
    SETUP_TOGGLE(SETUP_FLASH_SEATBELT_ALARM, "Belt alarm", DEFAULT_SEATBELT_ALARM,
                 settings_state.seatbelt_alarm_enabled),

    // Diagnostics and messages
    SETUP_TOGGLE(SETUP_FLASH_ROUTE_MESSAGES, "Route msgs", DEFAULT_ROUTE_MESSAGES,
                 settings_state.route_msg_enabled),
    SETUP_TOGGLE_ACTION(SETUP_FLASH_ESC_TC_CUSTOMIZER, "Allow ESC/TC", DEFAULT_ESC_TC_CUSTOMIZER,
                        settings_state.esc_tc_customizator_enabled, setup_action_esc_tc),
    SETUP_TOGGLE(SETUP_FLASH_DYNO, "Allow Dyno", DEFAULT_DYNO, settings_state.dyno_mode_master_enabled),
    SETUP_TOGGLE(SETUP_FLASH_ACC_VIRTUAL_PAD, "ACC pad", DEFAULT_ACC_VIRTUAL_PAD,
                 settings_state.acc_virtual_pad_enabled),
    SETUP_TOGGLE(SETUP_FLASH_BRAKES_OVERRIDE, "Allow brake", DEFAULT_BRAKES_OVERRIDE,
                 settings_state.front_brake_forcer_master),
    SETUP_TOGGLE(SETUP_FLASH_4WD_DISABLER, "Allow 4WD", DEFAULT_4WD_DISABLER,
                 settings_state.awd_disabler_enabled),
    SETUP_TOGGLE(SETUP_FLASH_CLEAR_FAULTS, "Allow clear", DEFAULT_CLEAR_FAULTS,
                 settings_state.clear_faults_enabled),
    SETUP_TOGGLE(SETUP_FLASH_READ_FAULTS, "Allow read", DEFAULT_READ_FAULTS,
                 settings_state.read_faults_enabled),
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_REMOTE_START, DEFAULT_REMOTE_START, settings_state.remote_start_enabled),
    SETUP_TOGGLE_RENDER_ACTION(SETUP_FLASH_DIESEL_PARAMS, "Engine profile", DEFAULT_DIESEL_PARAMS,
                               settings_state.is_diesel_enabled, setup_render_diesel_params,
                               setup_action_diesel_params),

    // Driver assistance and comfort
    SETUP_TOGGLE_ACTION(SETUP_FLASH_ODOMETER_BLINK, "Stop odo blink", DEFAULT_ODOMETER_BLINK,
                        settings_state.disable_odometer_blink, setup_action_odometer_blink),
    SETUP_VALUE8_ACTION(SETUP_FLASH_PEDAL_BOOSTER, "Pedal mode", 8, DEFAULT_PEDAL_BOOSTER,
                        settings_state.pedal_booster_enabled, setup_render_pedal_booster,
                        setup_action_pedal_booster),
    SETUP_NUMBER(SETUP_FLASH_PEDAL_POWER, "Pedal trim", 255, DEFAULT_PEDAL_POWER,
                 SETUP_VALUE_INT8_AS_UINT8, settings_state.pedal_map_power, setup_render_pedal_power, -10, 2,
                 setup_action_pedal_power),
    {SETUP_FLASH_PARK_MIRROR, 1, DEFAULT_PARK_MIRROR, SETUP_VALUE_UINT8,
     &settings_state.park_mirror, "Park mirror", 0, UI_ENTRY_SUBMENU, 0, 0, 0, 0},
    SETUP_VALUE8_ACTION(SETUP_FLASH_ACC_AUTOSTART, "ACC resume", 2, DEFAULT_ACC_AUTOSTART,
                        settings_state.acc_autostart, setup_render_acc_autostart, setup_action_acc_autostart),
    SETUP_VALUE8_ACTION(SETUP_FLASH_CLOSE_WINDOWS, "Close", 2, DEFAULT_CLOSE_WINDOWS,
                        settings_state.close_windows_with_door_lock, setup_render_close_windows,
                        setup_action_close_windows),
    SETUP_VALUE8_ACTION(SETUP_FLASH_OPEN_WINDOWS, "Open", 2, DEFAULT_OPEN_WINDOWS,
                        settings_state.open_windows_with_door_lock, setup_render_open_windows,
                        setup_action_open_windows),
    SETUP_TOGGLE_ACTION(SETUP_FLASH_HAS_VIRTUAL_PAD, "Allow HAS", DEFAULT_HAS_VIRTUAL_PAD,
                        settings_state.has_function_enabled, setup_action_has_virtual_pad),
    SETUP_TOGGLE(SETUP_FLASH_QV_EXHAUST_FLAP, "Allow exhaust", DEFAULT_QV_EXHAUST_FLAP,
                 settings_state.qv_exhaust_flap_function_enabled),
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_EUJOT, DEFAULT_EUJOT, settings_state.eujot_enabled),

    SETUP_TOGGLE(SETUP_FLASH_PDC_MUTE, "PDC mute", 0, settings_state.parking_sensor_mute),
    SETUP_TOGGLE(SETUP_FLASH_REVERSE_AUDIO, "Reverse mute", 0, settings_state.reverse_audio_mute),
    SETUP_TOGGLE(SETUP_FLASH_ROTATE, "Auto rotate", 0, settings_state.rotate_readings),

    {34, 1, 0, SETUP_VALUE_UINT8, &settings_state.usb_sniffer,
     "USB mode", setup_render_usb_mode, UI_ENTRY_EXCLUSIVE_MODE, 0, 0, 0, setup_action_usb_mode},
    SETUP_HIDDEN_TOGGLE(35, 0, settings_state.usb_elm327),

    SETUP_TOGGLE(37, "Advanced", 0, settings_state.advanced_pages),
    SETUP_HIDDEN_TOGGLE(36, 0, settings_state.gasoline_v6),

    // Hidden persisted values
    SETUP_HIDDEN_TOGGLE(SETUP_FLASH_SHOW_RACE_MASK, DEFAULT_SHOW_RACE_MASK, settings_state.show_race_mask),
};
const uint8_t setup_params_count = sizeof(setup_params) / sizeof(setup_params[0]);

/* Replace the current setting text and clear any leftover characters. */
static void setup_write_text(uint8_t start, const char *text) {
    uint8_t col = start;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && *text)
        dashboard_setup_screen[col++] = (uint8_t)*text++;
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH)
        dashboard_setup_screen[col++] = ' ';
}

static void setup_write_value(const char *label, const char *value) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_value(text, sizeof(text), label, value);
    setup_write_text(0, text);
}
static void setup_write_number(const char *label, int value) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_number(text, sizeof(text), label, value, false);
    setup_write_text(0, text);
}

/* Keep the original flash flags while exposing one exclusive USB personality. */
static void setup_action_usb_mode(void) {
    if (ibs_override_enabled()) {
        menu_notice(UI_SYMBOL_WARNING " Stop IBS first");
        return;
    }
    if (settings_state.usb_sniffer) {
        settings_state.usb_sniffer = 0;
#ifdef ACT_AS_ELM327
        settings_state.usb_elm327 = 1;
#else
        settings_state.usb_elm327 = 0;
#endif
    } else if (settings_state.usb_elm327) {
        settings_state.usb_elm327 = 0;
    } else {
        settings_state.usb_sniffer = 1;
    }
}

static void setup_render_usb_mode(void) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    const char *mode = settings_state.usb_sniffer ? "CAN" : "OFF";
#ifdef ACT_AS_ELM327
    if (!settings_state.usb_sniffer && settings_state.usb_elm327)
        mode = "ELM327";
#endif
    ui_render_value(text, sizeof(text), "USB mode", mode);
    setup_write_text(0, text);
}

/* Toggle the automatic engine-stop blocking preference. */
static void setup_action_start_stop(void) {
    settings_state.smart_disable_start_stop_enabled = !settings_state.smart_disable_start_stop_enabled;
    comfort_state.request_to_disable_start_and_stop = 0;
}

/* Toggle custom stability-control behavior and notify the other boards. */
static void setup_action_esc_tc(void) {
    settings_state.esc_tc_customizator_enabled = !settings_state.esc_tc_customizator_enabled;

    uint8_t msg[2] = {C2_Bh_BusID, C2_Bh_cmdFunction_ESC_TC_Enabled};
    if (!settings_state.esc_tc_customizator_enabled) {
        chassis_state.stability_inverted = 0;
        msg[1] = C2_Bh_cmdFunction_ESC_TC_Disabled;
    }
    board_uart_send(msg, 2);
}

/* Cycle I4 gasoline, V6 gasoline and diesel without changing saved page identities. */
static void setup_action_diesel_params(void) {
    if (settings_state.is_diesel_enabled) {
        settings_state.is_diesel_enabled = 0;
        settings_state.gasoline_v6 = 0;
    } else if (!settings_state.gasoline_v6) {
        settings_state.gasoline_v6 = 1;
    } else {
        settings_state.is_diesel_enabled = 1;
    }
    parameter_page_count = settings_state.is_diesel_enabled ? diesel_page_count : gasoline_page_count;
}

/* Toggle suppression of the dashboard's blinking odometer. */
static void setup_action_odometer_blink(void) {
    settings_state.disable_odometer_blink = !settings_state.disable_odometer_blink;

    uint8_t msg[2] = {BhBusID, BHcmdOdometerBlinkDefault};
    if (settings_state.disable_odometer_blink)
        msg[1] = BHcmdOdometerBlinkDisable;
    board_uart_send(msg, 2);
}

/* Select the next accelerator-response mode and notify the other boards. */
static void setup_action_pedal_booster(void) {
    settings_state.pedal_booster_enabled++;
    if (settings_state.pedal_booster_enabled > 8)
        settings_state.pedal_booster_enabled = 0;
    if (settings_state.pedal_booster_enabled == 0)
        pedal_booster_set_map(2);

    uint8_t msg[3] = {C2_Bh_BusID, C2_Bh_cmdSetPedalBoostStatus, settings_state.pedal_booster_enabled};
    board_uart_send(msg, 3);
}

/* Adjust the selected pedal-response trim and request a fresh map status. */
static void setup_action_pedal_power(void) {
    pedal_state.current_schizzaforte_map = '-';
}

/* Select how adaptive cruise resumes after a stop. */
static void setup_action_acc_autostart(void) {
    settings_state.acc_autostart++;
    if (settings_state.acc_autostart > 2)
        settings_state.acc_autostart = 0;
}

/* Select the lock-button gesture that closes the windows. */
static void setup_action_close_windows(void) {
    settings_state.close_windows_with_door_lock++;
    if (settings_state.close_windows_with_door_lock > 2)
        settings_state.close_windows_with_door_lock = 0;
    comfort_state.close_windows_request = 0;
    comfort_state.door_locks_requests_counter = 0;
}

/* Select the unlock-button gesture that opens the windows. */
static void setup_action_open_windows(void) {
    settings_state.open_windows_with_door_lock++;
    if (settings_state.open_windows_with_door_lock > 2)
        settings_state.open_windows_with_door_lock = 0;
    comfort_state.open_windows_request = 0;
    comfort_state.door_unlocks_requests_counter = 0;
}

/* Toggle the virtual highway-assist button and notify the other boards. */
static void setup_action_has_virtual_pad(void) {
    settings_state.has_function_enabled = !settings_state.has_function_enabled;

    uint8_t msg[2] = {C2_Bh_BusID, C2_Bh_cmdFunctHAS_Disabled};
    if (settings_state.has_function_enabled)
        msg[1] = C2_Bh_cmdFunctHAS_Enabled;
    board_uart_send(msg, 2);
}

/* Show the selected launch-assist torque threshold. */
static void setup_render_launch_torque(void) {
    setup_write_number("Launch Nm", settings_state.launch_torque_threshold);
}

/* Show the selected shift-indicator engine speed. */
static void setup_render_shift_rpm(void) {
    setup_write_number("Shift RPM", settings_state.shift_threshold);
}

/* Show the engine profile used by the parameter menu. */
static void setup_render_diesel_params(void) {
    setup_write_value("Engine", settings_state.is_diesel_enabled ? "2.2 D"
                               : settings_state.gasoline_v6 ? "2.9 V6" : "2.0 I4");
}

/* Show the selected accelerator-response mode. */
static void setup_render_pedal_booster(void) {
    static const char *const labels[] = {"OFF",   "Auto",   "Bypass",
                                         "A", "N",  "D",
                                         "R", "Hybrid", "Kids"};
    uint8_t index = settings_state.pedal_booster_enabled;
    setup_write_value("Pedal mode", labels[index <= 8 ? index : 0]);
}

/* Show the signed pedal-response trim. */
static void setup_render_pedal_power(void) {
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    ui_render_signed_number(text, sizeof(text), "Pedal trim", settings_state.pedal_map_power, false);
    setup_write_text(0, text);
}

/* Show how adaptive cruise is configured to resume. */
static void setup_render_acc_autostart(void) {
    static const char *const labels[] = {"OFF", "RES", "+"};
    uint8_t index = settings_state.acc_autostart;
    setup_write_value("ACC resume", labels[index <= 2 ? index : 0]);
}

/* Show the lock-button gesture required to close the windows. */
static void setup_render_close_windows(void) {
    static const char *const labels[] = {"OFF", "1 lock", "2 locks"};
    uint8_t index = settings_state.close_windows_with_door_lock;
    setup_write_value("Close", labels[index <= 2 ? index : 0]);
}

/* Show the unlock-button gesture required to open the windows. */
static void setup_render_open_windows(void) {
    static const char *const labels[] = {"OFF", "1 unlock", "2 unlocks"};
    uint8_t index = settings_state.open_windows_with_door_lock;
    setup_write_value("Open", labels[index <= 2 ? index : 0]);
}

#endif /* BACCABLE_C1 */
