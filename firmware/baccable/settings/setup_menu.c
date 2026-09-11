/*
 * setup_menu.c
 *
 * Setup menu engine. Menu entries live in setup_entries.c.
 */

#include "settings/setup_menu.h"
#include "features/menu.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"

    #define SETUP_SAVE_EXIT_PAGE 0
    #define SETUP_SAVE_EXIT_TEXT "< Save and back"
    #define SETUP_MARK_TEXT_START 2

uint8_t setup_dashboardPageIndex = 0;
uint8_t dashboard_setup_screen[DASHBOARD_MESSAGE_MAX_LENGTH];
uint8_t total_pages_in_setup_dashboard_menu = 0;

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

/* Count the available feature settings, including the save-and-return entry. */
static uint8_t setup_menu_pages_count(void) {
    uint8_t count = 1; // page 0 is Save and back
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
    if (!page_index)
        return NULL;
    uint8_t visible_page = 1;
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

/* Jump to a different functional group of feature settings. */
void setup_move_group(int8_t delta) {
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
    const char *text =
        (page == SETUP_SAVE_EXIT_PAGE) ? SETUP_SAVE_EXIT_TEXT : (param ? param->menu_text : "");
    uint8_t col = (param && param->display_mode == SETUP_DISPLAY_STATUS_MARK) ? SETUP_MARK_TEXT_START : 0;

    memset(dashboard_setup_screen, ' ', DASHBOARD_MESSAGE_MAX_LENGTH);
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && text && *text)
        dashboard_setup_screen[col++] = (uint8_t)*text++;
}

/* Switch an on/off feature preference. */
static void setup_toggle_bool(const SetupParam *param) { setup_set_value(param, !setup_get_value(param)); }

/* Show whether the displayed feature preference is enabled. */
static void setup_render_checkbox(const SetupParam *param) {
    if (param->display_mode == SETUP_DISPLAY_STATUS_MARK)
        dashboard_setup_screen[0] = dashboard_state.checkbox_symbols[!!setup_get_value(param)];
}

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
    return stored_value;
}

/* Restore all feature preferences before vehicle operation starts. */
void setup_load_from_flash(void) {
    setup_menu_pages_count();
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value(&setup_params[i], settings_read(setup_params[i].flash_index));
}

/* Collect the current feature preferences for saving. */
void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = setup_get_value(&setup_params[i]);
}

/* Show the selected setting and its current value. */
void setup_render_page(uint8_t page_index) {
    if (page_index >= setup_menu_pages_count())
        return;

    setup_reset_page_text(page_index);

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    setup_render_checkbox(param);
    if (param->render)
        param->render();
}

/* Select the next or previous feature setting. */
void setup_move_page(int8_t delta) {
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
    /* Saving and returning are handled by the menu controller. */
    if (page_index == 0)
        return;

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    if (param->action) {
        param->action();
    } else if (param->value_type == SETUP_VALUE_UINT8 && param->max_value == 1) {
        setup_toggle_bool(param);
    }
}

#endif /* BACCABLE_C1 */
