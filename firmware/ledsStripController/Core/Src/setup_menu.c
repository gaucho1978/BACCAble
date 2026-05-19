/*
 * setup_menu.c
 *
 * To add a new simple on/off feature to the setup menu:
 *   1. globalVariables.h - add: extern uint8_t your_var;
 *   2. globalVariables.c - add: uint8_t your_var = 0;
 *                                and a label row in dashboard_setup_menu_array
 *   3. setup_menu.c      - add one SETUP_TOGGLE_AUTO_BOOL entry below
 *
 * For entries with side effects or multi-value state, use SETUP_TOGGLE_CUSTOM
 * and handle the page explicitly in processingMessage0x000002FA.c.
 */

#include "setup_menu.h"

#if defined(C1baccable)

#include "functions_C1baccable.h"  // readFromFlash, dashboard_setup_menu_array, checkbox_symbols

// Parameter table  { flash_index | menu_page | max_value | toggle_mode | &variable }
//
// flash_index : 1-based slot (matches readFromFlash argument)
// menu_page   : page index in dashboard_setup_menu_array, or SETUP_MENU_NO_PAGE
// max_value   : highest valid persisted value
// toggle_mode : AUTO_BOOL = table may toggle it; CUSTOM = caller handles it
const SetupParam setup_params[] = {
    // flash  page                    max  toggle                  variable
    {  2,  1,                 1, SETUP_TOGGLE_CUSTOM,    &function_smart_disable_start_stop_enabled  },
    {  3,  3,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_led_strip_controller_enabled      },
    {  4,  4,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_shift_indicator_enabled           },
    {  6,  6,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_ipc_my23_is_installed             },
    {  7,  9,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_route_msg_enabled                 },
    {  8, 11,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_dyno_mode_master_enabled          },
    {  9, 12,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_acc_virtual_pad_enabled           },
    { 10, 13,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_front_brake_forcer_master         },
    { 11, 14,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_4wd_disabler_enabled              },
    { 12, 17,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_remote_start_Enabled              },
    { 13, 15,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_clear_faults_enabled              },
    { 14, 10,                 1, SETUP_TOGGLE_CUSTOM,    &function_esc_tc_customizator_enabled       },
    { 15, 16,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_read_faults_enabled               },
    { 16, 18,                 1, SETUP_TOGGLE_CUSTOM,    &function_is_diesel_enabled                 },
    { 17,  7,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_regeneration_alert_enabled        },
    { 19,  8,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_seatbelt_alarm_enabled            },
    { 20, 20,                 6, SETUP_TOGGLE_CUSTOM,    &function_pedal_booster_enabled             },
    { 21, 19,                 1, SETUP_TOGGLE_CUSTOM,    &function_disable_odometer_blink            },
    { 22, SETUP_MENU_NO_PAGE, 1, SETUP_TOGGLE_NONE,      &function_show_race_mask                    },
    { 23, 22,                 1, SETUP_TOGGLE_CUSTOM,    &function_park_mirror                       },
    { 24, 23,                 2, SETUP_TOGGLE_CUSTOM,    &function_acc_autostart                     },
    { 25, 24,                 2, SETUP_TOGGLE_CUSTOM,    &function_close_windows_with_door_lock      },
    { 26, 25,                 2, SETUP_TOGGLE_CUSTOM,    &function_open_windows_with_door_lock       },
    { 27, 26,                 1, SETUP_TOGGLE_CUSTOM,    &HAS_function_enabled                       },
    { 28, 27,                 1, SETUP_TOGGLE_AUTO_BOOL, &QV_exhaust_flap_function_enabled           },
    { 30, 28,                 1, SETUP_TOGGLE_AUTO_BOOL, &function_eujot_enabled                     },
    // ADD NEW SIMPLE CHECKBOX HERE:
    // { NEW_FLASH_IDX, NEW_MENU_PAGE, 1, SETUP_TOGGLE_AUTO_BOOL, &new_function_variable },
};
const uint8_t setup_params_count = sizeof(setup_params) / sizeof(setup_params[0]);

const SetupParam *setup_find_by_flash_index(uint8_t flash_index) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index == flash_index)
            return &setup_params[i];
    return 0;
}

uint8_t setup_flash_slots_count(void) {
    uint8_t count = SETUP_FLASH_SLOTS;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index > count)
            count = setup_params[i].flash_index;
    return count;
}

void setup_load_from_flash(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        *setup_params[i].value = (uint8_t)readFromFlash(setup_params[i].flash_index);
}

void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = *setup_params[i].value;
}

uint8_t setup_is_dirty(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if ((uint16_t)*setup_params[i].value != readFromFlash(setup_params[i].flash_index))
            return 1;
    return 0;
}

uint8_t setup_update_checkbox(uint8_t page_index) {
    for (uint8_t i = 0; i < setup_params_count; i++) {
        if (setup_params[i].menu_page == page_index) {
            dashboard_setup_menu_array[page_index][0] = checkbox_symbols[!!(*setup_params[i].value)];
            return 1;
        }
    }
    return 0;
}

uint8_t setup_toggle_if_auto_bool(uint8_t page_index) {
    for (uint8_t i = 0; i < setup_params_count; i++) {
        if (setup_params[i].menu_page == page_index &&
            setup_params[i].toggle_mode == SETUP_TOGGLE_AUTO_BOOL &&
            setup_params[i].max_value == 1) {
            *setup_params[i].value = !(*setup_params[i].value);
            return 1;
        }
    }
    return 0;
}

#endif /* C1baccable */
