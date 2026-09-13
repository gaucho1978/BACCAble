#include "test_report.h"
#include "features/menu.h"
#include "features/ui_entry.h"
#include "vehicle/steering_controls.h"
#include "features/ibs_override.h"
#include "diagnostics/fault_reader.h"
#include "features/periodic.h"
#include "features/display_stream.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
#include "storage/flash_records.h"
#include <assert.h>
#include <stdio.h>

SettingsState settings_state;
SecurityState security_state;
ChassisState chassis_state;
ComfortState comfort_state;
TelemetryState telemetry_state;
DiagnosticsState diagnostics_state;
RuntimeState runtime_state;
DashboardState dashboard_state;
StatisticsState statistics_state;
PedalState pedal_state;
MirrorsState mirrors_state;
DisplayState display_state;
const char *FW_VERSION = "BACCABLE test";
static uint32_t now;
static char screen[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
static uint8_t old_visibility[30], saved[MENU_PREFS_SIZE];
static bool have_old, have_saved, fail_save;
static unsigned commands, queries;
static unsigned settings_writes, preference_writes, usb_applies;
static uint16_t fail_type;
static uint8_t last_command;
static bool uart_busy;
uint32_t HAL_GetTick(void) { return now; }
uint8_t board_uart_send(const uint8_t *data, size_t size) {
    if (uart_busy)
        return 0;
    if (data[0] == BhBusIDparamString) {
        assert(size == UART_BUFFER_SIZE);
        memcpy(screen, data + 1, DASHBOARD_MESSAGE_MAX_LENGTH);
        screen[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
    } else {
        ++commands;
        last_command = size > 1 ? data[1] : 0;
    }
    return 1;
}
void pedal_booster_set_map(uint8_t map) { (void)map; }
void _putchar(char c) { (void)c; }
void usb_modes_apply(void) { ++usb_applies; }
void status_led_error(void) {}
void status_led_activity(void) {}
uint32_t can_tx(CAN_TxHeaderTypeDef *header, uint8_t *data) {
    (void)header;
    (void)data;
    ++queries;
    return HAL_OK;
}
uint32_t can_forward(const CAN_RxHeaderTypeDef *header, uint8_t *data) {
    (void)header;
    (void)data;
    return HAL_OK;
}
float native_parameter_read(uint8_t id) { return (float)id + 0.5f; }
bool flash_record_load(unsigned slot, uint16_t type, void *data, size_t size) {
    (void)slot;
    if (type == 0x103 && have_old) {
        assert(size == sizeof(old_visibility));
        memcpy(data, old_visibility, size);
        return true;
    }
    if (type == 0x104 && have_saved) {
        assert(size == sizeof(saved));
        memcpy(data, saved, size);
        return true;
    }
    return false;
}
bool flash_record_save(unsigned slot, uint16_t type, const void *data, size_t size) {
    (void)slot;
    if (type == 0x101) ++settings_writes;
    if (type == 0x104) ++preference_writes;
    if (fail_save || type == fail_type)
        return false;
    if (type == 0x104) {
        assert(size == sizeof(saved));
        memcpy(saved, data, size);
        have_saved = true;
    }
    return true;
}
/* Distinguish clicks, holds and stream loss at exact boundaries, including tick wrap. */
static void test_input(void) {
    const uint32_t starts[] = {100, UINT32_MAX - 500};
    for (unsigned run = 0; run < 2; ++run) {
        uint32_t start = starts[run];
        MenuInput input = {0};
        assert(menu_input_update(&input, 0x90, true, start) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start) == MENU_NONE);
        assert(menu_input_update(&input, 0x90, true, start + 1) == MENU_NONE);
        for (unsigned t = 101; t < 1200; t += 100)
            assert(menu_input_update(&input, 0x90, true, start + t) == MENU_NONE);
        assert(menu_input_update(&input, 0x90, true, start + 1200) == MENU_NONE);
        assert(menu_input_update(&input, 0x90, true, start + 1201) == MENU_BACK);
        assert(menu_input_update(&input, 0x90, true, start + 1300) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start + 1400) == MENU_NONE);

        /* Releasing just before BACK is still exactly one SELECT. */
        start += 1500;
        assert(menu_input_update(&input, 0x50, true, start) == MENU_NONE);
        for (unsigned t = 100; t < 1200; t += 100)
            assert(menu_input_update(&input, 0x50, true, start + t) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start + 1199) == MENU_SELECT);
        assert(menu_input_update(&input, 0x10, true, start + 1200) == MENU_NONE);

        /* 300 ms gaps are tolerated; 301 ms cancels a lost gesture. */
        assert(menu_input_update(&input, 0x90, true, start + 1300) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start + 1600) == MENU_SELECT);
        assert(menu_input_update(&input, 0x90, true, start + 1700) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start + 2001) == MENU_NONE);
        assert(menu_input_update(&input, 0x90, true, start + 2100) == MENU_NONE);
        assert(menu_input_update(&input, 0x90, false, start + 2200) == MENU_NONE);
        assert(menu_input_update(&input, 0x10, true, start + 2300) == MENU_NONE);
    }
    MenuInput input = {0};
    menu_input_update(&input, 0x10, true, 10);
    assert(menu_input_update(&input, 0x18, true, 20) == MENU_NEXT);
    assert(menu_input_update(&input, 0x18, true, 30) == MENU_NONE);
    assert(menu_input_update(&input, 0x20, true, 40) == MENU_NEXT_GROUP);
    menu_input_update(&input, 0x10, true, 50);
    assert(menu_input_update(&input, 0x08, true, 60) == MENU_PREVIOUS);
    assert(menu_input_update(&input, 0x00, true, 70) == MENU_PREVIOUS_GROUP);
}
/* Apply queued fragments to a receiver that exposes each fragment immediately. */
static unsigned drain_display(DisplayStream *stream, uint8_t *visible) {
    uint8_t part, chars[3];
    unsigned count = 0;
    while (display_stream_peek(stream, &part, chars)) {
        assert(++count <= DISPLAY_FRAGMENT_COUNT);
        memcpy(visible + part * 3, chars, 3);
        display_stream_accept(stream);
    }
    return count;
}

/* Check latest-target updates, retry ownership, fairness and restoration at both widths. */
static void test_display(void) {
    DisplayStream stream = {0};
    uint8_t a[DASHBOARD_MESSAGE_MAX_LENGTH], b[sizeof(a)], c[sizeof(a)], visible[sizeof(a)];
    uint8_t part, chars[3];
    memset(a, 'A', sizeof(a));
    memset(b, 'B', sizeof(b));
    memset(c, 'C', sizeof(c));
    display_stream_submit(&stream, a);
    assert(drain_display(&stream, visible) == DISPLAY_FRAGMENT_COUNT);
    assert(!memcmp(visible, a, sizeof(a)));
    display_stream_submit(&stream, a);
    assert(!display_stream_peek(&stream, &part, chars));

    unsigned changed = DISPLAY_FRAGMENT_COUNT / 2;
    memcpy(a + changed * 3, "XYZ", 3);
    display_stream_submit(&stream, a);
    assert(display_stream_peek(&stream, &part, chars) && part == changed);
    assert(!memcmp(chars, "XYZ", 3));
    /* No accept after CAN failure: retry must retain the same pending fragment. */
    assert(display_stream_peek(&stream, &part, chars) && part == changed);
    assert(drain_display(&stream, visible) == 1);
    assert(!memcmp(visible, a, sizeof(a)));

    display_stream_reset(&stream);
    display_stream_submit(&stream, a);
    assert(display_stream_peek(&stream, &part, chars) && part == 0);
    memcpy(visible, chars, 3);
    display_stream_accept(&stream);
    display_stream_submit(&stream, b);
    assert(display_stream_peek(&stream, &part, chars) && part == 1 && chars[0] == 'B');
    assert(drain_display(&stream, visible) == DISPLAY_FRAGMENT_COUNT);
    assert(!memcmp(visible, b, sizeof(b)));

    display_stream_submit(&stream, a);
    assert(display_stream_peek(&stream, &part, chars));
    uint8_t offered_part = part;
    memcpy(visible + part * 3, chars, 3);
    display_stream_submit(&stream, c); /* The accepted snapshot must not become C retroactively. */
    display_stream_accept(&stream);
    assert(stream.dirty & (1U << offered_part));
    assert(drain_display(&stream, visible) == DISPLAY_FRAGMENT_COUNT);
    assert(!memcmp(visible, c, sizeof(c)));

    /* A failed obsolete fragment can be replaced by the newest target on retry. */
    display_stream_submit(&stream, a);
    assert(display_stream_peek(&stream, &part, chars));
    display_stream_submit(&stream, b);
    assert(display_stream_peek(&stream, &part, chars) && chars[0] == 'B');
    drain_display(&stream, visible);
    assert(!memcmp(visible, b, sizeof(b)));

    /* Changing the start continually must not starve the end of the line. */
    display_stream_reset(&stream);
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        b[0] = 'a' + i;
        display_stream_submit(&stream, b);
        assert(display_stream_peek(&stream, &part, chars) && part == i);
        memcpy(visible + part * 3, chars, 3);
        display_stream_accept(&stream);
    }
    drain_display(&stream, visible);
    assert(!memcmp(visible, b, sizeof(b)));

    memset(c, ' ', sizeof(c));
    memcpy(c, "Oil", 3);
    display_stream_submit(&stream, c);
    drain_display(&stream, visible);
    assert(!memcmp(visible, c, sizeof(c))); /* Clear the complete old suffix. */
    display_stream_refresh(&stream);
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        display_stream_submit(&stream, c);
        display_stream_refresh(&stream); /* Repeated factory traffic does not restart restoration. */
        assert(display_stream_peek(&stream, &part, chars));
        display_stream_accept(&stream);
    }
    assert(!display_stream_peek(&stream, &part, chars));
    memset(c, ' ', sizeof(c));
    display_stream_submit(&stream, c);
    drain_display(&stream, visible);
    display_stream_refresh(&stream);
    assert(!display_stream_peek(&stream, &part, chars));
    display_stream_submit(&stream, a);
    display_stream_reset(&stream);
    assert(!display_stream_peek(&stream, &part, chars));
}
static void test_preferences(void) {
    MenuPreferences prefs, copy;
    menu_preferences_default(&prefs);
    uint8_t list[64], raw[MENU_PREFS_SIZE];
    for (unsigned e = 0; e < 2; ++e) {
        assert(menu_page_list(&prefs, e, 0, true, false, list) == 4);
        unsigned total = 0;
        for (unsigned g = 1; g < MENU_GROUPS; ++g)
            total += menu_page_list(&prefs, e, g, false, false, list);
        assert(total == menu_page_count(e));
        for (unsigned i = 0; i < menu_page_count(e); ++i) {
            const ParameterPage *page = &parameter_pages[e][i];
            assert(menu_page_index(e, page->id) == (int)i);
            assert((page->id & 0x7f) > 0 && (page->id & 0x7f) <= 64);
            assert(page->label && page->group > 0 && page->group < MENU_GROUPS);
            for (unsigned j = 0; j < i; ++j)
                assert(parameter_pages[e][j].id != page->id);
        }
    }
    uint16_t first = prefs.favorites[0][0], second = prefs.favorites[0][1];
    menu_favorite_move(&prefs, 0, first, 1);
    assert(prefs.favorites[0][0] == second && prefs.favorites[0][1] == first);
    menu_page_show(&prefs, 0, menu_page_index(0, first), false);
    assert(menu_page_list(&prefs, 0, 0, true, false, list) == 4); /* Hide != remove favorite. */
    prefs.alphabetical = 1;
    unsigned n = menu_page_list(&prefs, 0, 0, false, false, list);
    for (unsigned i = 1; i < n; ++i)
        assert(strcmp(parameter_pages[0][list[i - 1]].label, parameter_pages[0][list[i]].label) <= 0);
    menu_preferences_encode(&prefs, raw);
    assert(menu_preferences_decode(&copy, raw));
    assert(!memcmp(&prefs, &copy, sizeof(prefs)));
    assert(!menu_page_visible(&copy, 0, menu_page_index(0, first)));
    for (unsigned i = 0; i < menu_page_count(0); ++i)
        menu_page_show(&copy, 0, i, false);
    assert(menu_page_list(&copy, 0, 0, false, false, list) == 0);
    assert(menu_page_list(&copy, 0, 0, false, true, list) == menu_page_count(0));
    raw[0] = 255;
    assert(!menu_preferences_decode(&copy, raw));
}
static void fresh_menu(void) {
    settings_state.is_diesel_enabled = 0;
    settings_state.gasoline_v6 = 0;
    settings_state.advanced_pages = 0;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    now = 1000;
    have_saved = false;
    have_old = false;
    fail_save = false;
    fail_type = 0;
    menu_init();
    menu_event(MENU_BACK);
}
static void to_settings(void) {
    menu_event(MENU_BACK); /* Favorites -> root. */
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
}
/* Persistence compares committed domains and retries only the unsuccessful work. */
static void test_automatic_persistence(void) {
    fresh_menu();
    to_settings();
    assert(settings_save() == 0 && menu_preferences_save() == 0);
    settings_writes = preference_writes = usb_applies = 0;
    runtime_state.instruct_slave_boards_trigger_enabled = 0;
    menu_event(MENU_SELECT); /* Setup. */
    menu_event(MENU_BACK); /* Unchanged exit. */
    assert(settings_writes == 0 && preference_writes == 0 && usb_applies == 0);
    assert(strstr(screen, "Features") && !strstr(screen, "Saved"));
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Sort changes preferences only. */
    menu_event(MENU_BACK);
    assert(settings_writes == 0 && preference_writes == 1 && usb_applies == 0);
    assert(!runtime_state.instruct_slave_boards_trigger_enabled);
    menu_event(MENU_BACK); /* Consecutive exit does not write again. */
    assert(preference_writes == 1);

    fresh_menu();
    to_settings();
    assert(settings_save() == 0 && menu_preferences_save() == 0);
    settings_writes = preference_writes = usb_applies = 0;
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Dirty preferences. */
    settings_state.shift_indicator_enabled = !settings_state.shift_indicator_enabled;
    fail_type = 0x104;
    menu_event(MENU_BACK);
    assert(settings_writes == 1 && preference_writes == 1 && usb_applies == 1);
    assert(strstr(screen, "Save failed"));
    now += 60001; menu_process();
    menu_event(MENU_NEXT); /* Error is modal; browsing cannot dismiss it. */
    assert(preference_writes == 1 && strstr(screen, "Save failed"));
    menu_event(MENU_SELECT); /* Still failing. */
    assert(settings_writes == 1 && preference_writes == 2 && usb_applies == 1);
    fail_type = 0;
    menu_event(MENU_SELECT);
    assert(preference_writes == 3 && settings_writes == 1 && usb_applies == 1);
    assert(strstr(screen, "Settings") && !strstr(screen, "Saved"));
    menu_event(MENU_BACK);
    assert(preference_writes == 3);

    fresh_menu();
    to_settings();
    assert(settings_save() == 0 && menu_preferences_save() == 0);
    settings_writes = preference_writes = usb_applies = 0;
    settings_state.shift_indicator_enabled = !settings_state.shift_indicator_enabled;
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Preferences must save even when settings fail. */
    fail_type = 0x101;
    menu_event(MENU_BACK);
    assert(strstr(screen, "Save failed") && usb_applies == 0);
    menu_event(MENU_BACK); /* Cancel exit, retain committed RAM value. */
    assert(strstr(screen, "Sort"));
    fail_type = 0;
    menu_event(MENU_BACK);
    assert(settings_writes == 2 && preference_writes == 1 && usb_applies == 1);
}

/* A rejected screen remains retryable, including a periodic resend of identical text. */
static void test_present_retry(void) {
    fresh_menu();
    menu_present("Old screen");
    uart_busy = true;
    menu_present("New screen");
    assert(!strncmp(screen, "Old screen", 10));
    uart_busy = false;
    menu_present("New screen");
    assert(!strncmp(screen, "New screen", 10));

    now += 500;
    uart_busy = true;
    menu_present("New screen");
    uart_busy = false;
    memset(screen, 0, sizeof(screen));
    menu_present("New screen");
    assert(!strncmp(screen, "New screen", 10));
    /* Successful submissions still suppress immediate duplicates. */
    memset(screen, 0, sizeof(screen));
    menu_present("New screen");
    assert(screen[0] == 0);
}
static void test_controller(void) {
    fresh_menu();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    assert(dashboard_state.dashboard_page_index == 3);
    menu_event(MENU_NEXT);
    uint8_t selected = dashboard_state.dashboard_page_index;
    menu_event(MENU_BACK);
    menu_event(MENU_BACK); /* Root, close + persist. */
    assert(!dashboard_state.baccable_dashboard_menu_visible && have_saved);
    menu_init();
    menu_event(MENU_BACK);
    assert(dashboard_state.dashboard_page_index == selected);
    fresh_menu();
    to_settings();
    menu_event(MENU_SELECT); /* Enter Feature setup. */
    fail_save = true;
    menu_event(MENU_BACK);
    assert(strstr(screen, "! Save failed"));
    now += 1800;
    menu_render();
    assert(strstr(screen, "! Save failed"));
    fail_save = false;
    menu_event(MENU_SELECT);
    assert(!strstr(screen, "Saved"));
    now += 1500;
    menu_render();
    assert(strstr(screen, "> Features"));
    /* Import old visibility, retaining the existing settings record. */
    memset(old_visibility, 0xff, sizeof(old_visibility));
    old_visibility[0] &= ~2;
    have_saved = false;
    have_old = true;
    menu_init();
    assert(menu_preferences_save() == 0);
    MenuPreferences decoded;
    assert(menu_preferences_decode(&decoded, saved));
    assert(!menu_page_visible(&decoded, 0, 1) && !menu_page_visible(&decoded, 1, 1));
    fresh_menu();
    menu_show_parameter(23); /* Gasoline battery voltage UDS. */
    now = 2000;
    menu_process();
    assert(queries);
    const ParameterDefinition *p = &parameter_definitions[35];
    assert(p->value_offset + p->value_length <= 4);
    uint16_t did = ((p->request_data >> 16) & 0xff) << 8 | (p->request_data >> 24);
    uint8_t reply[8] = {7, 0x62, did >> 8, did, 0, 0, 0, 10};
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = p->response_id, .DLC = 8};
    reply[3] ^= 1;
    parameter_request_receive(&header, reply);
    assert(isnan(parameter_cache_get(35, now)));
    reply[3] ^= 1;
    parameter_request_receive(&header, reply);
    assert(isfinite(parameter_cache_get(35, now)));
    now += 3001;
    menu_render();
    assert(strstr(screen, "--"));
    assert(isnan(parameter_cache_get(35, now)));
    parameter_cache_put(35, 14.2f, now);
    menu_render();
    assert(strstr(screen, "14.20"));
    parameter_cache_reset();
    assert(isnan(parameter_cache_get(35, now)));
    /* Data from an unrelated CAN frame cannot refresh an expired oil value. */
    parameter_cache_observe(0x4b2, 4);
    assert(isfinite(parameter_cache_get(5, now)));
    now += 3001;
    parameter_cache_observe(0x101, 3);
    assert(isnan(parameter_cache_get(5, now)));
    parameter_cache_observe(0x4b2, 3);
    assert(isnan(parameter_cache_get(5, now)));
}
static void test_navigation_regressions(void) {
    /* Rapid physical wheel reports update every selection while UART is busy. */
    fresh_menu();
    MenuPreferences defaults;
    menu_preferences_default(&defaults);
    uint8_t pages[256];
    unsigned count = menu_page_list_filtered(&defaults, 0, 1, true, false, false, false, pages);
    assert(count > 1);
    unsigned initial = 0;
    while (initial < count && pages[initial] != dashboard_state.dashboard_page_index)
        ++initial;
    assert(initial < count);
    menu_button(0x10, true);
    uart_busy = true;
    for (unsigned i = 1; i <= 3; ++i) {
        now += 20;
        menu_button(0x18, true);
        assert(dashboard_state.dashboard_page_index == pages[(initial + i) % count]);
        menu_button(0x10, true);
    }
    uart_busy = false;
    settings_state.rotate_readings = 1;
    uint8_t manual_page = dashboard_state.dashboard_page_index;
    now += 4999;
    menu_process();
    assert(dashboard_state.dashboard_page_index == manual_page);
    now += 1;
    menu_process();
    assert(dashboard_state.dashboard_page_index == pages[(initial + 4) % count]);
    settings_state.rotate_readings = 0;

    fresh_menu();
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Parameter groups. */
    menu_event(MENU_SELECT); /* Engine group. */
    menu_event(MENU_NEXT);
    uint8_t selected = dashboard_state.dashboard_page_index;
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Temperatures. */
    menu_event(MENU_BACK);
    menu_event(MENU_PREVIOUS);
    menu_event(MENU_SELECT);
    assert(dashboard_state.dashboard_page_index == selected);

    fresh_menu();
    to_settings();
    menu_event(MENU_SELECT);
    menu_event(MENU_NEXT); /* Browse a setting without committing it. */
    fail_save = true;
    menu_event(MENU_BACK);
    assert(strstr(screen, "! Save failed"));
    fail_save = false;
    menu_event(MENU_SELECT); /* Retry BACK, without toggling the setting. */
    now += 1500;
    menu_render();
    assert(strstr(screen, "> Features"));

    fresh_menu();
    settings_state.is_diesel_enabled = 1;
    menu_event(MENU_NEXT); /* A second input can arrive before menu_process. */
    assert(parameter_page_count == diesel_page_count);
    assert(dashboard_state.dashboard_page_index < diesel_page_count);
    assert(menu_parameters_active());
    settings_state.is_diesel_enabled = 0;
    menu_process();
    assert(parameter_page_count == gasoline_page_count && menu_parameters_active());

    fresh_menu();
    menu_show_parameter(23);
    now += 1000;
    menu_process();
    const ParameterDefinition *p = &parameter_definitions[35];
    uint16_t did = ((p->request_data >> 16) & 0xff) << 8 | (p->request_data >> 24);
    uint8_t reply[8] = {7, 0x62, did >> 8, did, 0, 0, 0, 10};
    CAN_RxHeaderTypeDef header = {.IDE = CAN_ID_EXT, .ExtId = p->response_id, .DLC = 8};
    menu_event(MENU_NEXT);
    parameter_request_receive(&header, reply);
    assert(isnan(parameter_cache_get(35, now))); /* Late answer from the previous page. */

    MenuPreferences prefs;
    menu_preferences_default(&prefs);
    memset(prefs.favorites, 0, sizeof(prefs.favorites));
    menu_page_show(&prefs, 0, 23, false);
    menu_preferences_encode(&prefs, saved);
    have_saved = true;
    dashboard_state.baccable_dashboard_menu_visible = 0;
    menu_init();
    menu_event(MENU_BACK);
    assert(strstr(screen, "No favorites"));
    menu_event(MENU_NEXT);
    assert(!menu_parameters_active());
    menu_show_parameter(23);
    assert(menu_parameters_active() && dashboard_state.dashboard_page_index == 23);
    assert(menu_preferences_save() == 0);
    assert(menu_preferences_decode(&prefs, saved));
    assert(!menu_page_visible(&prefs, 0, 23));
}
/* Re-enter a section without resetting its remembered action, setting or information page. */
static void test_navigation_context(void) {
    for (unsigned section = 2; section <= 4; ++section) {
        fresh_menu();
        settings_state.dyno_mode_master_enabled = 1;
        settings_state.qv_exhaust_flap_function_enabled = 1;
        chassis_state.dyno_mode_enabled_on_master = 0;
        menu_event(MENU_BACK); /* Favorites -> root. */
        for (unsigned i = 0; i < section; ++i)
            menu_event(MENU_NEXT);
        menu_event(MENU_SELECT);
        if (section == 2) {
            for (unsigned i = 0; i < 16 && !strstr(screen, "Dyno"); ++i)
                menu_event(MENU_NEXT);
            assert(strstr(screen, "Dyno"));
        } else {
            menu_event(MENU_NEXT);
            menu_event(MENU_NEXT);
            if (section == 3)
                assert(strstr(screen, "Shown pages"));
            else {
                menu_event(MENU_NEXT);
                assert(strstr(screen, "MY23:"));
            }
        }
        char selected[sizeof(screen)];
        memcpy(selected, screen, sizeof(selected));
        unsigned before = commands;
        menu_event(MENU_BACK);
        menu_event(MENU_SELECT);
        assert(!strcmp(screen, selected));
        assert(commands == before); /* Re-entry must never execute the remembered action. */

        if (section == 4) {
            menu_event(MENU_SELECT); /* Information SELECT is a status no-op. */
            menu_event(MENU_SELECT);
            assert(!strcmp(screen, selected));
        }
        menu_event(MENU_BACK);
        menu_event(MENU_BACK); /* Close and save, but retain RAM navigation context. */
        assert(!dashboard_state.baccable_dashboard_menu_visible);
        menu_event(MENU_BACK); /* Reopen in Favorites. */
        menu_event(MENU_BACK);
        for (unsigned i = 0; i < section; ++i)
            menu_event(MENU_NEXT);
        menu_event(MENU_SELECT);
        assert(!strcmp(screen, selected));
        assert(commands == before);

        if (section == 2) {
            menu_event(MENU_BACK);
            settings_state.dyno_mode_master_enabled = 0;
            menu_event(MENU_SELECT);
            assert(strncmp(screen, "Dyno:", 5)); /* A remembered action must still pass availability. */
            assert(commands == before);
        }
        settings_state.dyno_mode_master_enabled = 0;
        settings_state.qv_exhaust_flap_function_enabled = 0;
    }
}

/* Held directions repeat in lists, never in Actions or while moving a favorite. */
static void test_repeat_views(void) {
    fresh_menu();
    to_settings();
    menu_button(0x10, true);
    menu_button(0x18, true);
    assert(strstr(screen, "Favorites"));
    now += 250;
    menu_button(0x18, true);
    now += 250;
    menu_button(0x18, true);
    assert(strstr(screen, "Shown pages"));
    now += 180;
    menu_button(0x18, true);
    assert(strstr(screen, "Fav. order"));
    menu_button(0x10, true);
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT); /* Pick a favorite for reordering. */
    menu_button(0x18, true);
    char selected[sizeof(screen)];
    memcpy(selected, screen, sizeof(selected));
    for (unsigned i = 0; i < 5; ++i) {
        now += 200;
        menu_button(0x18, true);
        assert(!strcmp(screen, selected));
    }

    fresh_menu();
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); /* Actions */
    menu_button(0x10, true);
    menu_button(0x18, true);
    memcpy(selected, screen, sizeof(selected));
    unsigned before = commands;
    for (unsigned i = 0; i < 5; ++i) {
        now += 200;
        menu_button(0x18, true);
        assert(!strcmp(screen, selected) && commands == before);
    }
}

/* Feedback durations differ without preventing navigation from dismissing a notice. */
static void test_notice_timing(void) {
    fresh_menu();
    menu_notice("Maximum hold: ON");
    now += 749;
    menu_render();
    assert(strstr(screen, "Maximum hold: ON"));
    now += 1;
    menu_render();
    assert(!strstr(screen, "Maximum hold: ON"));
    menu_notice("! Unavailable");
    now += 1799;
    menu_render();
    assert(strstr(screen, "! Unavailable"));
    now += 1;
    menu_render();
    assert(!strstr(screen, "! Unavailable"));
    menu_notice("! Unavailable");
    menu_event(MENU_NEXT);
    assert(!strstr(screen, "! Unavailable"));
}

/* Idle menus close, while readings, active operations and failed saves remain recoverable. */
static void test_idle_close(void) {
    fresh_menu();
    now += 60001;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible); /* Readings stay visible. */
    menu_event(MENU_BACK);
    now += 29999;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    now += 1;
    uart_busy = true;
    menu_process();
    assert(!dashboard_state.baccable_dashboard_menu_visible);
    uart_busy = false;
    menu_process();
    for (unsigned i = 0; i < DASHBOARD_MESSAGE_MAX_LENGTH; ++i)
        assert(screen[i] == ' '); /* Clear retried even while the menu is hidden. */
    menu_event(MENU_BACK);
    assert(dashboard_state.baccable_dashboard_menu_visible);
    menu_process();
    assert(screen[0] != ' '); /* An old pending clear cannot erase a reopened menu. */

    fresh_menu();
    menu_event(MENU_BACK);
    uart_busy = true;
    now += 30000;
    menu_process();
    assert(!dashboard_state.baccable_dashboard_menu_visible);
    uart_busy = false;
    menu_event(MENU_BACK); /* Reopen before the pending clear could be retried. */
    char reopened[sizeof(screen)];
    memcpy(reopened, screen, sizeof(reopened));
    menu_process();
    assert(!strcmp(screen, reopened));

    fresh_menu();
    to_settings();
    now += 59999;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    fail_save = true;
    now += 1;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible && strstr(screen, "Save failed"));
    fail_save = false;
    now += 60001;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible); /* Wait for deliberate retry. */
    menu_event(MENU_SELECT); /* Retry completes the original idle close. */
    assert(!dashboard_state.baccable_dashboard_menu_visible);

    fresh_menu();
    menu_event(MENU_BACK);
    diagnostics_state.clear_faults_request = 1;
    now += 30001;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    diagnostics_state.clear_faults_request = 0;
    now += 29999;
    menu_process();
    assert(dashboard_state.baccable_dashboard_menu_visible);
    now += 1;
    menu_process();
    assert(!dashboard_state.baccable_dashboard_menu_visible);

    fresh_menu();
    now = UINT32_MAX - 1000;
    menu_event(MENU_BACK);
    now += 30000;
    menu_process();
    assert(!dashboard_state.baccable_dashboard_menu_visible);
}

static void expect_reading(uint8_t engine, uint16_t id, float first, float second, const char *expected) {
    int index = menu_page_index(engine, id);
    assert(index >= 0);
    const ParameterPage *page = &parameter_pages[engine][index];
    float values[] = {first, second};
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    dashboard_format_values(page->name, values, page->parameter_ids, text);
    assert(!strcmp(text, expected));
}
static void expect_setup(uint8_t id, const char *expected) {
    const SetupParam *param = setup_find_by_flash_index(id);
    assert(param && param->render);
    memset(dashboard_setup_screen, 'X', sizeof(dashboard_setup_screen));
    param->render();
    assert(!memcmp(dashboard_setup_screen, expected, strlen(expected)));
    for (unsigned i = strlen(expected); i < sizeof(dashboard_setup_screen); ++i)
        assert(dashboard_setup_screen[i] == ' ');
}
static void test_readable_screens(void) {
    fresh_menu();
    expect_reading(0, 0x12, 100, 100, "Oil temp 100 C");
    expect_reading(1, 0x98, 100, 100, "Oil temp 100 C");
    expect_reading(0, 0x20, 90, 90, "Coolant temp  90C");
    expect_reading(1, 0xa0, -20, -20, "Coolant temp -20C");
    expect_reading(0, 0x18, 14.2f, 14.2f, "Battery 14.20 V");
    expect_reading(0, 0x17, -12.3f, -12.3f, "Battery  -12.3 A");
    expect_reading(0, 0x07, 14.2f, -12.3f, "Batt 14.2V  -12.3A");
    expect_reading(0, 0x07, 12.1f, -150.5f, "Batt 12.1V -150.5A");
    expect_reading(0, 0x04, 100, 90, "Oil 100C Cool  90C");
    expect_reading(0, 0x02, 1.2f, 90, "Oil1.2bar Cool 90C");
    expect_reading(1, 0x8d, 650, 650, "DPF temp  650 C");
    expect_reading(1, 0x99, 12.34f, 12.34f, "Oil press 12.34bar");
    expect_reading(1, 0xa6, -0.65f, -0.65f, "Turbo  -0.65 bar");
    expect_reading(1, 0xa8, -0.5f, -0.5f, "Boost req -0.50bar");
    expect_reading(1, 0xa6, -33.768f, -33.768f, "Turbo -33.77 bar");
    expect_reading(0, 0x18, NAN, NAN, "Battery    -- V");
    expect_reading(0, 0x2d, 41, 41, "Best100-200 MISS");

    settings_state.launch_torque_threshold = 25;
    expect_setup(18, "Launch Nm: 25");
    settings_state.launch_torque_threshold = 600;
    expect_setup(18, "Launch Nm: 600");
    settings_state.shift_threshold = 4500;
    expect_setup(5, "Shift RPM: 4500");
    settings_state.is_diesel_enabled = 0;
    expect_setup(16, "Engine: 2.0 I4");
    settings_state.is_diesel_enabled = 1;
    expect_setup(16, "Engine: 2.2 D");
    settings_state.pedal_map_power = -10;
    expect_setup(29, "Pedal trim: -10");
    settings_state.pedal_map_power = 10;
    expect_setup(29, "Pedal trim: +10");
    settings_state.close_windows_with_door_lock = 2;
    expect_setup(25, "Close: 2 locks");
    settings_state.open_windows_with_door_lock = 0;
    expect_setup(26, "Open: OFF");
    settings_state.pedal_booster_enabled = 2;
    expect_setup(20, "Pedal mode: Bypass");

    const char *short_templates[] = {"", "$", "$1", "$1.", "$1.0"};
    float values[] = {0, 0};
    const uint8_t ids[] = {1, 1};
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    for (unsigned i = 0; i < sizeof(short_templates) / sizeof(short_templates[0]); ++i) {
        dashboard_format_values(short_templates[i], values, ids, text);
        assert(!strcmp(text, short_templates[i]));
    }
}
/* Engine and complexity filters preserve saved identities, including temporarily unavailable favorites. */
static void test_engine_filters(void) {
    MenuPreferences prefs, restored;
    uint8_t raw[MENU_PREFS_SIZE], list[64];
    menu_preferences_default(&prefs);
    assert(menu_favorite_toggle(&prefs, 0, 0x2f));
    assert(menu_favorite_toggle(&prefs, 0, 0x22));
    menu_preferences_encode(&prefs, raw);
    assert(menu_preferences_decode(&restored, raw));
    assert(!memcmp(prefs.favorites, restored.favorites, sizeof(prefs.favorites)));
    for (unsigned v6 = 0; v6 < 2; ++v6) {
        unsigned count = menu_page_list_filtered(&prefs, 0, 0, false, false, v6, true, list);
        unsigned ignition = 0;
        for (unsigned i = 0; i < count; ++i) {
            unsigned id = parameter_pages[0][list[i]].id;
            assert(menu_page_index(0, id) == list[i]);
            ignition += (id >= 0x23 && id <= 0x26) || id == 0x2f || id == 0x30;
            assert(v6 || (id != 0x2f && id != 0x30));
            assert(!v6 || (id != 0x14 && id != 0x40));
        }
        assert(ignition == (v6 ? 6 : 4));
        count = menu_page_list_filtered(&prefs, 0, 0, true, false, v6, false, list);
        assert(count == (v6 ? 6 : 5)); /* Advanced favorite stays; cylinder 5 is temporarily filtered. */
    }
    unsigned count = menu_page_list_filtered(&prefs, 1, 0, false, false, false, true, list);
    assert(count == diesel_page_count);
    for (unsigned i = 0; i < count; ++i)
        assert(parameter_pages[1][list[i]].id >= 0x81);
    count = menu_page_list_filtered(&prefs, 0, 0, false, false, false, false, list);
    for (unsigned i = 0; i < count; ++i)
        assert(!menu_page_advanced(0, list[i]));
    assert(menu_page_list_filtered(&prefs, 0, 0, false, true, false, false, list) == gasoline_page_count - 2);
    assert(menu_page_index(0, 0x2f) == 46 && menu_page_index(0, 0x30) == 47);
    assert(parameter_page_elements(&parameter_pages[0][menu_page_index(0, 0x3b)]) == 2);
    assert(parameter_page_elements(&parameter_pages[0][menu_page_index(0, 0x3c)]) == 2);
    assert(parameter_page_elements(&parameter_pages[0][menu_page_index(0, 0x3f)]) == 2);
    memset(prefs.favorites[0], 0, sizeof(prefs.favorites[0]));
    prefs.favorites[0][0] = 0x04;
    prefs.favorites[0][1] = 0x2f;
    prefs.favorites[0][2] = 0x01;
    menu_favorite_move_supported(&prefs, 0, 0x04, 1, false);
    assert(prefs.favorites[0][0] == 0x01 && prefs.favorites[0][1] == 0x2f && prefs.favorites[0][2] == 0x04);
    assert(setup_read_flash_value(36, 0xffff) == 0 && setup_read_flash_value(37, 0xffff) == 0);
    assert(setup_read_flash_value(36, 0) == 0 && setup_read_flash_value(36, 1) == 1);
    assert(setup_read_flash_value(36, 2) == 0);
    fresh_menu();
    const SetupParam *profile = setup_find_by_flash_index(16);
    profile->action();
    assert(!settings_state.is_diesel_enabled && settings_state.gasoline_v6);
    expect_setup(16, "Engine: 2.9 V6");
    uint16_t values[SETUP_FLASH_PARAM_BUFFER_SIZE] = {0};
    settings_state.advanced_pages = 1;
    setup_fill_flash_params(values);
    assert(values[15] == 0 && values[35] == 1 && values[36] == 1);
    profile->action();
    assert(settings_state.is_diesel_enabled);
    profile->action();
    assert(!settings_state.is_diesel_enabled && !settings_state.gasoline_v6);
}

/* Only permitted vehicle actions appear; preferences still control their original runtime gates. */
static void test_action_availability(void) {
    uint8_t *gates[] = {
        &settings_state.dyno_mode_master_enabled,    &settings_state.awd_disabler_enabled,
        &settings_state.esc_tc_customizator_enabled, &settings_state.qv_exhaust_flap_function_enabled,
        &settings_state.has_function_enabled,        &settings_state.front_brake_forcer_master,
        &settings_state.read_faults_enabled,         &settings_state.clear_faults_enabled};
    const char *names[] = {"Dyno",       "4WD",         "ESC/TC",          "QV exhaust",
                           "HAS button", "Brake", "BCM faults", "Clear DTCs"};
    for (unsigned i = 0; i < 8; ++i)
        *gates[i] = 0;
    fresh_menu();
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    assert(strstr(screen, "Actions"));
    menu_event(MENU_SELECT);
    for (unsigned gate = 0; gate < 8; ++gate) {
        for (unsigned i = 0; i < 16; ++i) {
            menu_event(MENU_NEXT);
            assert(!strstr(screen, names[gate]));
            assert(!strstr(screen, "Immobilizer"));
        }
        *gates[gate] = 1;
        chassis_state.front_brake_forced = 0;
        chassis_state.launch_assist_enabled = 0;
        chassis_state.stability_inverted = 0;
        chassis_state.dyno_mode_enabled_on_master = gate == 5;
        telemetry_state.current_speed_km_h = 0;
        runtime_state.car_steady_counter = 100;
        bool found = false;
        for (unsigned i = 0; i < 16; ++i) {
            menu_event(MENU_NEXT);
            found |= strstr(screen, names[gate]) != NULL;
        }
        assert(found);
        *gates[gate] = 0;
    }
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT); /* Settings */
    menu_event(MENU_NEXT); /* Information */
    menu_event(MENU_SELECT);
    for (unsigned i = 0; i < 4; ++i)
        menu_event(MENU_NEXT);
    assert(strstr(screen, "Immobil") && (strstr(screen, ": ON") || strstr(screen, ": OFF")));
}

/* Action wording must describe requests without claiming vehicle confirmation. */
static void test_action_request_labels(void) {
    fresh_menu();
    runtime_state.car_steady_counter = 100;
    settings_state.awd_disabler_enabled = 1;
    settings_state.qv_exhaust_flap_function_enabled = 1;
    chassis_state.awd_sequence = 0;
    comfort_state.force_q_vexhaust_valve_opened = 0;
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    for (unsigned i = 0; i < 16 && !strstr(screen, "QV exhaust"); ++i)
        menu_event(MENU_NEXT);
    assert(strstr(screen, "> QV exhaust"));
    for (unsigned state = 1; state <= 4; ++state) {
        comfort_state.force_q_vexhaust_valve_opened = state;
        now += 1;
        menu_render();
        const char *expected = state == 4 ? "QV req: AUTO WAIT" : "QV req: OPEN WAIT";
        assert(!strncmp(screen, expected, strlen(expected)));
        for (unsigned i = strlen(expected); i < DASHBOARD_MESSAGE_MAX_LENGTH; ++i)
            assert(screen[i] == ' ');
    }
    comfort_state.force_q_vexhaust_valve_opened = 0;
    menu_render();
    assert(strstr(screen, "> QV exhaust"));
    for (unsigned i = 0; i < 16 && !strstr(screen, "4WD"); ++i)
        menu_event(MENU_NEXT);
    assert(strstr(screen, "> 4WD"));
    for (unsigned state = 1; state <= 4; ++state) {
        chassis_state.awd_sequence = state;
        menu_render();
        assert(strstr(screen, "4WD req: OFF WAIT"));
    }
    now = 12000; /* Periodic warning must also avoid claiming confirmed disablement. */
    menu_render();
    assert(strstr(screen, "! 4WD OFF request"));
    chassis_state.awd_sequence = 0;
    menu_render();
    assert(strstr(screen, "> 4WD"));
    settings_state.clear_faults_enabled = 1;
    for (unsigned i = 0; i < 16 && !strstr(screen, "Clear DTCs"); ++i)
        menu_event(MENU_NEXT);
    assert(strstr(screen, "> Clear DTCs"));
    diagnostics_state.clear_faults_request = 255;
    menu_render();
    assert(strstr(screen, "Clear DTCs: WAIT"));
    diagnostics_state.clear_faults_request = 0;
    menu_render();
    assert(strstr(screen, "> Clear DTCs"));
    settings_state.clear_faults_enabled = 0;
    settings_state.awd_disabler_enabled = 0;
    settings_state.qv_exhaust_flap_function_enabled = 0;
}

static void test_cache_flags(void) {
    parameter_cache_reset();
    const uint8_t ids[] = {0, 7, 8, 31, 32, 95, 96, 99};
    for (unsigned i = 0; i < sizeof(ids); ++i) {
        assert(isnan(parameter_cache_get(ids[i], 0)));
        parameter_cache_put(ids[i], i + 0.5f, UINT32_MAX - 100);
    }
    parameter_cache_put(7, NAN, UINT32_MAX - 100);
    parameter_cache_put(96, INFINITY, UINT32_MAX - 100);
    for (unsigned i = 0; i < sizeof(ids); ++i) {
        float value = parameter_cache_get(ids[i], 100);
        if (ids[i] == 7 || ids[i] == 96)
            assert(isnan(value));
        else
            assert(value == i + 0.5f);
        assert(isnan(parameter_cache_get(ids[i], 3000)));
    }
    parameter_cache_put(255, 1, 0);
    assert(isnan(parameter_cache_get(255, 0)));
    parameter_cache_reset();
    assert(isnan(parameter_cache_get(99, 0)));
}
/* Hold numerical peaks without freezing status readings or showing stale measurements. */
static void test_maximum_hold(void) {
    parameter_cache_reset();
    parameter_peak_enable(true);
    parameter_cache_put(4, -12.0f, 100);
    parameter_cache_put(4, -20.0f, 200);
    assert(parameter_cache_get(4, 200) == -12.0f);
    parameter_cache_put(8, 3, 100);
    parameter_cache_put(8, 1, 200);
    assert(parameter_cache_get(8, 200) == 1);
    assert(isnan(parameter_cache_get(4, 3201)));
    parameter_peak_reset();
    parameter_cache_put(4, -30.0f, 3300);
    assert(parameter_cache_get(4, 3300) == -30.0f);
    parameter_peak_enable(false);
    parameter_cache_put(4, -40.0f, 3400);
    assert(parameter_cache_get(4, 3400) == -40.0f);
    float values[] = {1, 2, 3, 4};
    uint8_t ids[] = {91, 92, 93, 94};
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    dashboard_format_values("$1.0f/$1.0f/$1.0f/$1.0f", values, ids, text);
    assert(!strcmp(text, "1/2/3/4"));
}

/* Retry auxiliary-board preferences after a busy link without losing or duplicating a setting. */
static void test_board_sync_retry(void) {
    now = 20000;
    runtime_state.all_processors_wakeup_time = 0;
    commands = 0;
    board_sync_restart();
    for (unsigned option = 0; option < 8; ++option) {
        uart_busy = true;
        board_sync_process();
        assert(commands == option);
        uart_busy = false;
        board_sync_process();
        assert(commands == option + 1);
    }
    board_sync_process();
    assert(!runtime_state.instruct_slave_boards_trigger_enabled);
    board_sync_process();
    assert(commands == 8);
}

#include "test_setup_ui.c"
#include "test_unified_ui.c"
#include "test_menu_contract.c"

int main(void) {
#ifdef MENU_DIAGNOSTICS
    const char *suite = "menu-debug";
#elif defined(LARGE_DISPLAY)
    const char *suite = "menu-24";
#else
    const char *suite = "menu-18";
#endif
    const HostTest tests[] = {
        HOST_TEST(test_setup_ui),
        HOST_TEST(test_shared_renderers),
        HOST_TEST(test_navigation_contract),
        HOST_TEST(test_visible_position_contract),
        HOST_TEST(test_reading_position_integrity),
        HOST_TEST(test_position_renderer_bounds),
        HOST_TEST(test_filtered_editor_positions),
        HOST_TEST(test_workflow_and_version_positions),
        HOST_TEST(test_pending_action_feedback),
        HOST_TEST(test_conditional_actions),
        HOST_TEST(test_fault_action_exclusion),
        HOST_TEST(test_numeric_menu_flow),
        HOST_TEST(test_permission_guards),
        HOST_TEST(test_usb_legacy_and_dependency),
        HOST_TEST(test_steering_menu_ownership),
        HOST_TEST(test_request_cancel_and_failure),
#ifdef MENU_DIAGNOSTICS
        HOST_TEST(test_hidden_diagnostics),
#endif
        HOST_TEST(test_input),
        HOST_TEST(test_display),
        HOST_TEST(test_preferences),
        HOST_TEST(test_present_retry),
        HOST_TEST(test_controller),
        HOST_TEST(test_automatic_persistence),
        HOST_TEST(test_navigation_regressions),
        HOST_TEST(test_navigation_context),
        HOST_TEST(test_repeat_views),
        HOST_TEST(test_notice_timing),
        HOST_TEST(test_idle_close),
        HOST_TEST(test_readable_screens),
        HOST_TEST(test_action_request_labels),
        HOST_TEST(test_cache_flags),
        HOST_TEST(test_maximum_hold),
        HOST_TEST(test_board_sync_retry),
        HOST_TEST(test_engine_filters),
        HOST_TEST(test_action_availability)
    };
    host_tests_run(suite, tests, sizeof(tests) / sizeof(tests[0]));
    puts("PASS: menu gestures, stable views, favorites, sorting, migration, save failure, UDS freshness");
}
