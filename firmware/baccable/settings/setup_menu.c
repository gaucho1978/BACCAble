/*
 * setup_menu.c
 *
 * Setup menu engine. Menu entries live in setup_entries.c.
 */

#include "settings/setup_menu.h"
#include "features/menu.h"
#include "features/ibs_override.h"
#include "diagnostics/fault_reader.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"



uint8_t setup_dashboardPageIndex = 0;
uint8_t dashboard_setup_screen[DASHBOARD_MESSAGE_MAX_LENGTH];
uint8_t total_pages_in_setup_dashboard_menu = 0;

/* Draft values never enter persisted state until SELECT accepts them. */
static const SetupParam *editing;
static int16_t draft;
static bool park_menu;
static uint8_t park_page;
static const struct {
    UiEntryType type;
    const char *label;
} park_entries[] = {{UI_ENTRY_TOGGLE, "Enabled"},
                    {UI_ENTRY_CAPTURE, "Store position"},
                    {UI_ENTRY_ACTION, "Back"}};
/* 0: browse, 1: confirm capture, 2: queued, 3: send rejected. */
static uint8_t park_capture;

/* Identify a draft or nested capture workflow before leaving setup. */
bool setup_in_workflow(void) { return editing != NULL || park_menu; }
/* Discard unaccepted edits when setup is reset or closes through inactivity. */
void setup_cancel_edit(void) {
    editing = NULL;
    park_menu = false;
    park_page = park_capture = 0;
}
/* Return one workflow level without accepting a draft or capturing a position. */
bool setup_back(void) {
    if (editing) {
        editing = NULL;
        return true;
    }
    if (park_capture) {
        park_capture = 0;
        return true;
    }
    if (park_menu) {
        park_menu = false;
        return true;
    }
    return false;
}

/* Parking capture has no storage acknowledgement on the inter-board protocol. */
static void setup_park_select(void) {
    if (park_capture == 2) {
        park_capture = 0;
    } else if (park_capture) {
        uint8_t command[2] = {BhBusID, BHcmdFunctParkMirrorStoreCurPos};
        park_capture = board_uart_send(command, sizeof(command)) ? 2 : 3;
    } else if (park_entries[park_page].type == UI_ENTRY_TOGGLE) {
        uint8_t command[2] = {BhBusID, settings_state.park_mirror ? BHcmdFunctParkMirrorDisabled
                                                               : BHcmdFunctParkMirrorEnabled};
        if (board_uart_send(command, sizeof(command)))
            settings_state.park_mirror = !settings_state.park_mirror;
        else
            menu_notice(UI_SYMBOL_WARNING " Send busy; retry");
    } else if (park_entries[park_page].type == UI_ENTRY_CAPTURE && settings_state.park_mirror) {
        park_capture = 1;
    } else if (park_entries[park_page].type == UI_ENTRY_ACTION) {
        park_menu = false;
    }
}

/* Keep enable, capture confirmation and queue feedback visually distinct. */
static void setup_park_render(char *text, size_t size) {
    if (park_capture == 1)
        ui_render_action(text, size, "Adjust; SELECT");
    else if (park_capture == 2)
        ui_render_value(text, size, "Store", "queued");
    else if (park_capture == 3)
        ui_render_unavailable(text, size, "Send busy; retry");
    else if (park_entries[park_page].type == UI_ENTRY_TOGGLE)
        ui_render_toggle(text, size, park_entries[park_page].label, settings_state.park_mirror);
    else if (park_entries[park_page].type == UI_ENTRY_CAPTURE && !settings_state.park_mirror)
        ui_render_unavailable(text, size, "Enable first");
    else if (park_entries[park_page].type == UI_ENTRY_CAPTURE)
        ui_render_action(text, size, park_entries[park_page].label);
    else
        snprintf_(text, size, UI_SYMBOL_BACK " Back");
}

/* Read the current value of a configurable feature. */
static uint16_t setup_get_value(const SetupParam *param) {
    switch (param->value_type) {
    case SETUP_VALUE_UINT16:
        return *(uint16_t *)param->value;
    case SETUP_VALUE_INT8_AS_UINT8:
        return (uint8_t)*(int8_t *)param->value;
    case SETUP_VALUE_UINT8:
    default:
        return *(uint8_t *)param->value;
    }
}

/* Restore a configurable feature's value using its declared type. */
static void setup_set_value(const SetupParam *param, uint16_t value) {
    switch (param->value_type) {
    case SETUP_VALUE_UINT16:
        *(uint16_t *)param->value = value;
        break;
    case SETUP_VALUE_INT8_AS_UINT8:
        *(int8_t *)param->value = (int8_t)(uint8_t)value;
        break;
    case SETUP_VALUE_UINT8:
    default:
        *(uint8_t *)param->value = (uint8_t)value;
        break;
    }
}

/* Check whether a saved preference has a user-facing menu entry. */
static uint8_t setup_param_is_visible(const SetupParam *param) { return param->menu_text != 0; }

/* Count the available feature settings, without synthetic navigation entries. */
static uint8_t setup_menu_pages_count(void) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_param_is_visible(&setup_params[i]))
            count++;
    if (count > SETUP_FLASH_PARAM_BUFFER_SIZE)
        count = SETUP_FLASH_PARAM_BUFFER_SIZE;
    total_pages_in_setup_dashboard_menu = count;
    return count;
}

/* Place a feature setting in its functional navigation group. */
static uint8_t setup_group(uint8_t id) {
    switch (id) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 17:
    case 33:
    case 37:
        return 0; /* Display */
    case 19:
    case 21:
    case 23:
    case 25:
    case 26:
    case 28:
    case 31:
    case 32:
        return 1; /* Comfort */
    case 9:
    case 24:
    case 27:
        return 2; /* Assistance */
    case 8:
    case 10:
    case 11:
    case 14:
    case 18:
    case 20:
    case 29:
        return 3; /* Drivetrain */
    default:
        return 4; /* Device */
    }
}

/* Find the feature setting at a visible menu position. */
static const SetupParam *setup_find_by_page(uint8_t page_index) {
    uint8_t visible_page = 0;
    for (unsigned group = 0; group < 5; ++group)
        for (unsigned i = 0; i < setup_params_count; ++i) {
            const SetupParam *param = &setup_params[i];
            if (!setup_param_is_visible(param) || setup_group(param->flash_index) != group)
                continue;
            if (visible_page++ == page_index)
                return param;
        }
    return NULL;
}

/* Nested lists have their own position; unfinished edits have no peer counter. */
bool setup_list_position(unsigned *current, unsigned *total) {
    const SetupParam *param = setup_find_by_page(setup_dashboardPageIndex);
    if (editing || park_capture || (param && setup_get_value(param) && menu_setting_busy(param->flash_index)))
        return false;
    *total = park_menu ? 3 : setup_menu_pages_count();
    *current = (park_menu ? park_page : setup_dashboardPageIndex) + 1;
    return *total != 0 && *current <= *total;
}

/* Jump to a different functional group of feature settings. */
void setup_move_group(int8_t delta) {
    if (setup_in_workflow()) {
        setup_move_page(delta);
        return;
    }
    const SetupParam *current = setup_find_by_page(setup_dashboardPageIndex);
    uint8_t group = current ? setup_group(current->flash_index) : 255;
    for (unsigned i = 0; i < setup_menu_pages_count(); ++i) {
        setup_move_page(delta);
        const SetupParam *next = setup_find_by_page(setup_dashboardPageIndex);
        if (!next || setup_group(next->flash_index) != group)
            return;
    }
}

/* Prepare a clean screen for the selected setting. */
static void setup_reset_page_text(uint8_t page) {
    const SetupParam *param = setup_find_by_page(page);
    const char *text = param ? param->menu_text : "";
    uint8_t col = 0;

    memset(dashboard_setup_screen, ' ', DASHBOARD_MESSAGE_MAX_LENGTH);
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && text && *text)
        dashboard_setup_screen[col++] = (uint8_t)*text++;
}

/* Switch an on/off feature preference. */
static void setup_toggle_bool(const SetupParam *param) { setup_set_value(param, !setup_get_value(param)); }

/* Find a setting by the permanent identity used in saved preferences. */
const SetupParam *setup_find_by_flash_index(uint8_t flash_index) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index == flash_index)
            return &setup_params[i];
    return 0;
}

/* Determine how many saved setting positions the current feature set needs. */
uint8_t setup_flash_slots_count(void) {
    uint8_t count = SETUP_FLASH_SLOTS;
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_params[i].flash_index > count)
            count = setup_params[i].flash_index;
    return count;
}

/* Restore a supported setting value or fall back to its default. */
uint16_t setup_read_flash_value(uint8_t flash_index, uint16_t stored_value) {
    const SetupParam *param = setup_find_by_flash_index(flash_index);
    if (!param)
        return 0;
    if (stored_value == 0xFFFF || stored_value > param->max_value)
        return param->default_value;
    if (param->entry_type == UI_ENTRY_NUMBER) {
        int value = param->value_type == SETUP_VALUE_INT8_AS_UINT8 ? (int8_t)(uint8_t)stored_value : stored_value;
        if (value < param->minimum || value > param->maximum)
            return param->default_value;
    }
    return stored_value;
}

/* Restore all feature preferences before vehicle operation starts. */
void setup_load_from_flash(void) {
    setup_menu_pages_count();
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value(&setup_params[i], settings_read(setup_params[i].flash_index));
    /* CAN capture already has runtime precedence over ELM327. */
    if (settings_state.usb_sniffer)
        settings_state.usb_elm327 = 0;
}

/* Collect the current feature preferences for saving. */
void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = setup_get_value(&setup_params[i]);
}

/* Refuse permission changes that would hide an active feature or silently reset another one. */
static const char *setup_unavailable(const SetupParam *param) {
    if (setup_get_value(param) && menu_setting_busy(param->flash_index))
        return "Request pending";
    switch (param->flash_index) {
    case 8:
        return settings_state.dyno_mode_master_enabled && chassis_state.dyno_mode_enabled_on_master
                   ? "Stop Dyno" : NULL;
    case 10:
        return settings_state.front_brake_forcer_master && chassis_state.front_brake_forced
                   ? "Release brk" : NULL;
    case 11:
        return settings_state.awd_disabler_enabled && chassis_state.awd_sequence ? "Cancel 4WD" : NULL;
    case 13:
        return settings_state.clear_faults_enabled && diagnostics_state.clear_faults_request ? "Clear active" : NULL;
    case 15:
        return settings_state.read_faults_enabled && fault_reader_busy() ? "Read active" : NULL;
    case 34:
        return ibs_override_enabled() ? "Stop IBS" : NULL;
    case 14:
        return settings_state.esc_tc_customizator_enabled && chassis_state.stability_inverted
                   ? "Reset ESC" : NULL;
    case 28:
        return settings_state.qv_exhaust_flap_function_enabled && comfort_state.force_q_vexhaust_valve_opened
                   ? "Release QV" : NULL;
    default: return NULL;
    }
}

/* Show the selected setting and its current value. */
void setup_render_page(uint8_t page_index) {
    if (page_index >= setup_menu_pages_count())
        return;

    setup_reset_page_text(page_index);

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    const char *reason = setup_unavailable(param);
    if (reason)
        ui_render_unavailable(text, sizeof(text), reason);
    else if (park_menu)
        setup_park_render(text, sizeof(text));
    else if (param->entry_type == UI_ENTRY_NUMBER && param->value_type == SETUP_VALUE_INT8_AS_UINT8)
        ui_render_signed_number(text, sizeof(text), param->menu_text,
                                editing == param ? draft : *(int8_t *)param->value, editing == param);
    else if (param->entry_type == UI_ENTRY_NUMBER)
        ui_render_number(text, sizeof(text), param->menu_text,
                         editing == param ? draft : (param->value_type == SETUP_VALUE_INT8_AS_UINT8
                             ? *(int8_t *)param->value : setup_get_value(param)), editing == param);
    else if (param->entry_type == UI_ENTRY_TOGGLE)
        ui_render_toggle(text, sizeof(text), param->menu_text, !!setup_get_value(param));
    else if (param->entry_type == UI_ENTRY_SUBMENU)
        ui_render_action(text, sizeof(text), param->menu_text);
    else {
        if (param->render)
            param->render();
        return;
    }
    memset(dashboard_setup_screen, ' ', sizeof(dashboard_setup_screen));
    memcpy(dashboard_setup_screen, text, strlen(text));
}

/* Select the next or previous feature setting. */
void setup_move_page(int8_t delta) {
    if (editing) {
        int value = draft + delta * editing->step;
        draft = value < editing->minimum ? editing->minimum : value > editing->maximum ? editing->maximum : value;
        return;
    }
    if (park_menu) {
        if (!park_capture)
            park_page = (uint8_t)((park_page + (delta > 0 ? 1 : 2)) % 3);
        return;
    }
    uint8_t pages_count = setup_menu_pages_count();
    if (pages_count == 0)
        return;

    int16_t page = (int16_t)setup_dashboardPageIndex + delta;
    while (page < 0)
        page += pages_count;
    while (page >= pages_count)
        page -= pages_count;

    setup_dashboardPageIndex = (uint8_t)page;
}

/* Apply the user's choice to the selected feature setting. */
void setup_select_page(uint8_t page_index) {
    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    const char *reason = setup_unavailable(param);
    if (reason) {
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_unavailable(text, sizeof(text), reason);
        menu_notice(text);
        return;
    }
    if (park_menu) {
        setup_park_select();
    } else if (editing == param) {
        setup_set_value(param, (uint16_t)draft);
        editing = NULL;
        if (param->action)
            param->action();
    } else if (param->entry_type == UI_ENTRY_NUMBER) {
        editing = param;
        draft = param->value_type == SETUP_VALUE_INT8_AS_UINT8 ? *(int8_t *)param->value : setup_get_value(param);
        if (draft < param->minimum) draft = param->minimum;
        if (draft > param->maximum) draft = param->maximum;
    } else if (param->entry_type == UI_ENTRY_SUBMENU) {
        park_menu = true;
        park_page = park_capture = 0;
    } else if (param->action) {
        param->action();
    } else if (param->value_type == SETUP_VALUE_UINT8 && param->max_value == 1) {
        setup_toggle_bool(param);
    }
}

#endif /* BACCABLE_C1 */
