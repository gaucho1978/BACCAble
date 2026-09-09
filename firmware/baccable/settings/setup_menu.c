/*
 * setup_menu.c
 *
 * Setup menu engine. Menu entries live in setup_menu_entries.c.
 */

#include "settings/setup_menu.h"

#if defined(BACCABLE_C1)

    #include <string.h>
    #include "app/powertrain.h"

    #define SETUP_SAVE_EXIT_PAGE 0
    #define SETUP_SAVE_EXIT_TEXT "SAVE&EXIT"
    #define SETUP_MARK_TEXT_START 3

uint8_t setup_dashboardPageIndex = 0;
uint8_t dashboard_setup_menu_array[SETUP_FLASH_PARAM_BUFFER_SIZE][DASHBOARD_MESSAGE_MAX_LENGTH];
uint8_t total_pages_in_setup_dashboard_menu = 0;

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

static uint8_t setup_param_is_visible(const SetupParam *param) { return param->menu_text != 0; }

static uint8_t setup_menu_pages_count(void) {
    uint8_t count = 1; // page 0 is SAVE&EXIT
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_param_is_visible(&setup_params[i]))
            count++;
    if (count > SETUP_FLASH_PARAM_BUFFER_SIZE)
        count = SETUP_FLASH_PARAM_BUFFER_SIZE;
    total_pages_in_setup_dashboard_menu = count;
    return count;
}

static const SetupParam *setup_find_by_page(uint8_t page_index) {
    if (page_index == SETUP_SAVE_EXIT_PAGE)
        return 0;

    uint8_t visible_page = 1;
    for (uint8_t i = 0; i < setup_params_count; i++) {
        if (!setup_param_is_visible(&setup_params[i]))
            continue;
        if (visible_page == page_index)
            return &setup_params[i];
        visible_page++;
    }
    return 0;
}

static void setup_reset_page_text(uint8_t page) {
    const SetupParam *param = setup_find_by_page(page);
    const char *text =
        (page == SETUP_SAVE_EXIT_PAGE) ? SETUP_SAVE_EXIT_TEXT : (param ? param->menu_text : "");
    uint8_t col = (param && param->display_mode == SETUP_DISPLAY_STATUS_MARK) ? SETUP_MARK_TEXT_START : 0;

    memset(dashboard_setup_menu_array[page], ' ', DASHBOARD_MESSAGE_MAX_LENGTH);
    while (col < DASHBOARD_MESSAGE_MAX_LENGTH && text && *text)
        dashboard_setup_menu_array[page][col++] = (uint8_t)*text++;
}

static void setup_toggle_bool(const SetupParam *param) { setup_set_value(param, !setup_get_value(param)); }

static void setup_render_checkbox(const SetupParam *param, uint8_t page) {
    if (param->display_mode == SETUP_DISPLAY_STATUS_MARK)
        dashboard_setup_menu_array[page][0] = dashboard_state.checkbox_symbols[!!setup_get_value(param)];
}

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

uint16_t setup_read_flash_value(uint8_t flash_index, uint16_t stored_value) {
    const SetupParam *param = setup_find_by_flash_index(flash_index);
    if (!param)
        return 0;
    if (stored_value == 0xFFFF || stored_value > param->max_value)
        return param->default_value;
    return stored_value;
}

void setup_load_from_flash(void) {
    setup_menu_pages_count();
    for (uint8_t i = 0; i < setup_params_count; i++)
        setup_set_value(&setup_params[i], settings_read(setup_params[i].flash_index));
}

void setup_fill_flash_params(uint16_t *params) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        params[setup_params[i].flash_index - 1] = setup_get_value(&setup_params[i]);
}

uint8_t setup_is_dirty(void) {
    for (uint8_t i = 0; i < setup_params_count; i++)
        if (setup_get_value(&setup_params[i]) != settings_read(setup_params[i].flash_index))
            return 1;
    return 0;
}

void setup_render_page(uint8_t page_index) {
    if (page_index >= setup_menu_pages_count())
        return;

    setup_reset_page_text(page_index);

    const SetupParam *param = setup_find_by_page(page_index);
    if (!param)
        return;

    setup_render_checkbox(param, page_index);
    if (param->render)
        param->render(page_index);
}

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

void setup_select_page(uint8_t page_index) {
    if (page_index == 0) {
        if (setup_is_dirty())
            settings_save();
        dashboard_state.dashboard_menu_indent_level = 0;
        return;
    }

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
