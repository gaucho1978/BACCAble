#include "features/menu.h"
#include "features/ui_entry.h"
#include "features/menu_diagnostics.h"
#include "diagnostics/fault_reader.h"
#include "features/ibs_override.h"
#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include "diagnostics/parameter_request.h"
#include "storage/flash_records.h"
#if defined(BACCABLE_C1)

typedef enum {
    ROOT,
    FAVORITES,
    GROUPS,
    VALUES,
    FUNCTIONS,
    SETTINGS,
    SETUP,
    EDIT_FAVORITES,
    EDIT_VISIBLE,
    ORDER_FAVORITES,
    INFO,
    FAULTS
#ifdef MENU_DIAGNOSTICS
    , DIAGNOSTICS
#endif
} MenuView;
typedef enum {
    ACTION_READ,
    ACTION_CLEAR,
    ACTION_DYNO,
    ACTION_ESC,
    ACTION_BRAKE,
    ACTION_AWD,
    ACTION_HAS,
    ACTION_EXHAUST,
    ACTION_STATS,
    ACTION_PEAK,
    ACTION_IBS,
    ACTION_LAUNCH,
    ACTION_COUNT
} MenuAction;
typedef struct {
    MenuAction id;
    uint8_t group;
    const char *name;
    UiEntryType type;
} ActionEntry;
static const ActionEntry actions[] = {
    {ACTION_EXHAUST, 0, "QV exhaust", UI_ENTRY_ACTION},
    {ACTION_HAS, 1, "HAS button", UI_ENTRY_ACTION},
    {ACTION_ESC, 1, "ESC/TC", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_DYNO, 2, "Dyno", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_BRAKE, 2, "Front brake", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_LAUNCH, 2, "End launch", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_AWD, 2, "4WD", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_READ, 3, "BCM faults", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_CLEAR, 3, "Clear DTCs", UI_ENTRY_CONDITIONAL_ACTION},
    {ACTION_STATS, 3, "Reset times", UI_ENTRY_ACTION},
    {ACTION_PEAK, 3, "Peak hold", UI_ENTRY_TOGGLE},
    {ACTION_IBS, 3, "IBS override", UI_ENTRY_CONDITIONAL_ACTION}
};
typedef enum { REQUEST_NONE, REQUEST_WAIT, REQUEST_SENT, REQUEST_FAILED, REQUEST_TIMEOUT } RequestState;
typedef struct {
    uint32_t started;
    uint8_t state;
    bool target;
} ActionRequest;
static ActionRequest requests[ACTION_COUNT];
#define ACTION_TIMEOUT_MS 10000U
#ifdef MENU_DIAGNOSTICS
#define INFO_PAGES 6U
#else
#define INFO_PAGES 5U
#endif
static const char *const roots[] = {"Favorites", "Readings", "Actions", "Settings", "Information"};
static const char *const settings[] = {"Features",   "Favorites", "Shown pages",
                                       "Fav. order", "Sort order"};
static char peer_versions[2][DASHBOARD_MESSAGE_MAX_LENGTH - 2];
static uint32_t peer_updated[2];
static uint8_t peer_seen[2];

/* Remember a board's version and when it last responded. */
void menu_peer_status(uint8_t peer, const uint8_t *version) {
    if (peer > 1)
        return;
    memcpy(peer_versions[peer], version, sizeof(peer_versions[peer]) - 1);
    peer_versions[peer][sizeof(peer_versions[peer]) - 1] = 0;
    peer_updated[peer] = currentTime;
    peer_seen[peer] = 1;
}
static MenuPreferences preferences;
static MenuInput input;
static MenuView view = FAVORITES;
static uint8_t gasoline_v6, advanced_pages;
static uint8_t root, group = 1, function, setting, info, editor_page, engine;
static uint8_t list[64], list_count, selection, order_selected;
static uint8_t setup_last, fault_index;
static uint32_t page_changed, last_render, last_query, notice_started;
static const char *notice;
static char notice_text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
static uint8_t previous_text[DASHBOARD_MESSAGE_MAX_LENGTH], previous_valid;
static uint32_t previous_sent;
static uint8_t confirmed_action = 255;
static bool close_pending;
/* A failed exit stays explicit until the user retries or cancels it. */
static bool save_failed, save_close;
static MenuView save_destination;
static uint8_t saved_preferences[MENU_PREFS_SIZE];
static bool preferences_saved;
#define MENU_IDLE_MS 30000U
#define EDITOR_IDLE_MS 60000U
#define NOTICE_CONFIRM_MS 750U
#define NOTICE_WARNING_MS 1800U
#define NOTICE_REQUEST_MS 1200U
static uint32_t notice_duration;
static uint32_t confirm_started, last_input, info_changed;
static void build_pages(uint16_t selected);

/* Move through a list and continue from the other end at its boundary. */
static unsigned wrap(unsigned value, unsigned count, int delta) {
    if (!count)
        return 0;
    return (value + count + (delta < 0 ? count - 1 : 1)) % count;
}

/* Check whether a vehicle action is enabled by the user's preferences. */
static bool available(MenuAction id) {
    switch (id) {
    case ACTION_READ:
        return settings_state.read_faults_enabled;
    case ACTION_CLEAR:
        return settings_state.clear_faults_enabled;
    case ACTION_DYNO:
        return settings_state.dyno_mode_master_enabled;
    case ACTION_ESC:
        return settings_state.esc_tc_customizator_enabled;
    case ACTION_BRAKE:
    case ACTION_LAUNCH:
        return settings_state.front_brake_forcer_master;
    case ACTION_AWD:
        return settings_state.awd_disabler_enabled;
    case ACTION_HAS:
        return settings_state.has_function_enabled;
    case ACTION_EXHAUST:
        return settings_state.qv_exhaust_flap_function_enabled;
    default:
        return true;
    }
}

/* Keep a feature permission available until its in-flight menu request resolves. */
bool menu_setting_busy(uint8_t flash_index) {
    MenuAction id;
    switch (flash_index) {
    case 8: id = ACTION_DYNO; break;
    case 10: id = ACTION_BRAKE; break;
    case 13: id = ACTION_CLEAR; break;
    case 14: id = ACTION_ESC; break;
    case 27: id = ACTION_HAS; break;
    default: return false;
    }
    return requests[id].state == REQUEST_WAIT;
}

/* Explain the same known conditions before selection and immediately before execution. */
static const char *action_unavailable(MenuAction id) {
    if (!available(id))
        return "Disabled in setup";
    if (requests[id].state == REQUEST_WAIT)
        return "Request pending";
    switch (id) {
    case ACTION_READ:
        return diagnostics_state.clear_faults_request ? "Clear active" : NULL;
    case ACTION_CLEAR:
        if (diagnostics_state.clear_faults_request)
            return "Clear active";
        return fault_reader_busy() ? "Read active" : NULL;
    case ACTION_IBS:
        return telemetry_state.current_rpm_speed <= 400 ? "Start engine" : NULL;
    case ACTION_DYNO:
        if (chassis_state.front_brake_forced)
            return "Release brk";
        if (chassis_state.stability_inverted)
            return "Reset ESC";
        return runtime_state.car_steady_counter < 100 ? "Stop car" : NULL;
    case ACTION_ESC:
        return chassis_state.dyno_mode_enabled_on_master || requests[ACTION_DYNO].state == REQUEST_WAIT
                   ? "Dyno active" : NULL;
    case ACTION_BRAKE:
        if (chassis_state.launch_assist_enabled)
            return "End launch";
        if (!chassis_state.front_brake_forced) {
            if (telemetry_state.current_speed_km_h != 0)
                return "Stop car";
            if (!chassis_state.dyno_mode_enabled_on_master)
                return "Enable Dyno";
        }
        return NULL;
    case ACTION_LAUNCH:
        return chassis_state.launch_assist_enabled ? NULL : "Launch OFF";
    case ACTION_AWD:
        return !chassis_state.awd_sequence && runtime_state.car_steady_counter < 100 ? "Stop car" : NULL;
    default:
        return NULL;
    }
}

/* Track only delivery/acknowledgement evidence; never infer physical vehicle success. */
static void action_requests_process(void) {
    for (unsigned id = 0; id < ACTION_COUNT; ++id) {
        ActionRequest *request = &requests[id];
        if (request->state != REQUEST_WAIT)
            continue;
        if ((id == ACTION_CLEAR && !diagnostics_state.clear_faults_request) ||
            (id == ACTION_HAS && !comfort_state.has_button_press_requested))
            request->state = REQUEST_SENT;
        else if (currentTime - request->started >= ACTION_TIMEOUT_MS)
            request->state = REQUEST_TIMEOUT;
    }
}

/* Finish a menu request only when its existing C2 acknowledgement arrives. */
void menu_action_reply(uint8_t command) {
    MenuAction id;
    bool enabled;
    switch (command) {
    case C1cmdDynoActive: id = ACTION_DYNO; enabled = true; break;
    case C1cmdDynoNotActive: id = ACTION_DYNO; enabled = false; break;
    case C1cmdForceFrontBrake: id = ACTION_BRAKE; enabled = true; break;
    case C1cmdNormalFrontBrake: id = ACTION_BRAKE; enabled = false; break;
    default: return;
    }
    if (requests[id].state == REQUEST_WAIT)
        requests[id].state = requests[id].target == enabled ? REQUEST_SENT : REQUEST_FAILED;
}

/* Mark a successfully queued request as pending without replaying it automatically. */
static void action_requested(MenuAction id, bool target) {
    requests[id] = (ActionRequest){currentTime, REQUEST_WAIT, target};
}

/* Select the next available action or action group. */
static void function_move(int direction, bool next_group) {
    uint8_t old_group = actions[function].group;
    for (unsigned i = 0; i < sizeof(actions) / sizeof(actions[0]); ++i) {
        function = wrap(function, sizeof(actions) / sizeof(actions[0]), direction);
        if (available(actions[function].id) && (!next_group || old_group != actions[function].group))
            return;
    }
}

/* Request the current screen while avoiding unnecessary repeated text updates. */
void menu_present(const char *text) {
    uint8_t message[UART_BUFFER_SIZE];
    memset(message, ' ', sizeof(message));
    message[0] = BhBusIDparamString;
    size_t length = strlen(text);
    if (length > DASHBOARD_MESSAGE_MAX_LENGTH)
        length = DASHBOARD_MESSAGE_MAX_LENGTH;
    memcpy(message + 1, text, length);
    if (previous_valid && !memcmp(previous_text, message + 1, sizeof(previous_text)) &&
        currentTime - previous_sent < 500)
        return;
    /* Remember only accepted screens so UART rejection does not delay the next attempt. */
    if (!board_uart_send(message, sizeof(message)))
        return;
    memcpy(previous_text, message + 1, sizeof(previous_text));
    previous_valid = 1;
    previous_sent = currentTime;
    if (!text[0])
        close_pending = false;
}

/* Keep every reading byte intact; dense pages get position context before live data. */
void menu_present_reading(const char *text) {
    char numbered[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    size_t used = ui_render_position(numbered, sizeof(numbered), selection + 1, list_count);
    size_t length = strlen(text);
    if (used && used + length < sizeof(numbered)) {
        memcpy(numbered + used, text, length + 1);
        menu_present(numbered);
    } else if (used && currentTime - page_changed < NOTICE_REQUEST_MS) {
        const ParameterPage *page = &parameter_pages[engine][dashboard_state.dashboard_page_index];
        ui_render_list_entry(numbered, sizeof(numbered), selection + 1, list_count, page->label);
        menu_present(numbered);
    } else
        menu_present(text);
}

/* Show brief feedback about a selection, action or save result. */
void menu_notice(const char *text) {
    snprintf_(notice_text, sizeof(notice_text), "%s", text);
    notice = notice_text;
    notice_started = currentTime;
    size_t length = strlen(text);
    bool toggle = (length >= 4 && !strcmp(text + length - 4, ": ON")) ||
                  (length >= 5 && !strcmp(text + length - 5, ": OFF")) ||
                  ((text[0] == UI_GLYPH_CHECKED || text[0] == UI_GLYPH_UNCHECKED) && text[1] == ' ');
    notice_duration = (text[0] == UI_SYMBOL_WARNING[0] || text[0] == UI_GLYPH_CROSS) ? NOTICE_WARNING_MS
                      : toggle || !strcmp(text, "Records cleared")
                          ? NOTICE_CONFIRM_MS : NOTICE_REQUEST_MS;
    if (dashboard_state.baccable_dashboard_menu_visible)
        menu_present(text);
}

/* Save favorites, visibility, sort order and remembered pages. */
uint8_t menu_preferences_save(void) {
    uint8_t data[MENU_PREFS_SIZE];
    menu_preferences_encode(&preferences, data);
    if (preferences_saved && !memcmp(data, saved_preferences, sizeof(data)))
        return 0;
    if (!flash_record_save(VISIBILITY_RECORD, 0x104, data, sizeof(data)))
        return 255;
    memcpy(saved_preferences, data, sizeof(data));
    preferences_saved = true;
    return 0;
}

/* Switch parameter catalogs and discard readings from the previous engine profile. */
void menu_engine_changed(void) {
    engine = !!settings_state.is_diesel_enabled;
    gasoline_v6 = !!settings_state.gasoline_v6;
    advanced_pages = !!settings_state.advanced_pages;
    parameter_page_count = menu_page_count(engine);
    list_count = 0;
    parameter_request_cancel();
    parameter_cache_reset();
    dashboard_state.dashboard_page_index = 0;
    if (view == FAVORITES || view == VALUES || view == EDIT_FAVORITES || view == EDIT_VISIBLE ||
        view == ORDER_FAVORITES)
        build_pages(view == FAVORITES ? preferences.last_favorite[engine] : preferences.last[engine][group]);
}

/* Restore the user's menu layout or start with useful defaults. */
void menu_init(void) {
    memset(&input, 0, sizeof(input));
    memset(requests, 0, sizeof(requests));
    setup_cancel_edit();
#ifdef MENU_DIAGNOSTICS
    menu_diagnostics_reset();
#endif
    view = FAVORITES;
    root = 0;
    group = 1;
    function = 0;
    setting = 0;
    info = 0;
    info_changed = currentTime;
    setup_last = 0;
    notice = NULL;
    confirmed_action = 255;
    previous_valid = 0;
    last_query = 0;
    last_render = 0;
    order_selected = 0;
    save_failed = save_close = close_pending = false;
    preferences_saved = false;
    last_input = currentTime;
    menu_preferences_default(&preferences);
    uint8_t data[MENU_PREFS_SIZE];
    if (!flash_record_load(VISIBILITY_RECORD, 0x104, data, sizeof(data)) ||
        !menu_preferences_decode(&preferences, data)) {
        /* Old visibility was shared by both engine profiles. Import it once;
           only saving the new record replaces it. Settings and mirrors are untouched. */
        uint8_t old[30];
        if (flash_record_load(VISIBILITY_RECORD, 0x103, old, sizeof(old)))
            for (unsigned e = 0; e < 2; ++e)
                for (unsigned i = 0; i < menu_page_count(e); ++i)
                    menu_page_show(&preferences, e, i, !!(old[i / 8] & (1U << (i % 8))));
    } else {
        memcpy(saved_preferences, data, sizeof(data));
        preferences_saved = true;
    }
    menu_engine_changed();
}

/* Check whether the user is editing favorites or page visibility. */
static bool is_editor(void) { return view == EDIT_FAVORITES || view == EDIT_VISIBLE; }

/* Check whether a visible screen currently needs vehicle readings. */
bool menu_parameters_active(void) {
    return dashboard_state.baccable_dashboard_menu_visible && (view == VALUES || view == FAVORITES) &&
           list_count;
}

/* Fill the current screen with its latest usable measurements. */
void menu_parameters_refresh(void) {
    if (dashboard_state.dashboard_page_index >= parameter_page_count)
        return;
    const ParameterPage *page = &parameter_pages[engine][dashboard_state.dashboard_page_index];
    for (unsigned i = 0; i < parameter_page_elements(page); ++i)
        displayed_parameter_values[i] = parameter_cache_get(page->parameter_ids[i], currentTime);
}

/* Remember the selected reading and stop requests belonging to the previous page. */
static void select_page(void) {
    if (!list_count) {
        parameter_request_cancel();
        return;
    }
    uint8_t index = list[selection];
    if (is_editor()) {
        editor_page = index;
        return;
    }
    dashboard_state.dashboard_page_index = index;
    uint16_t id = parameter_pages[engine][index].id;
    if (view == FAVORITES)
        preferences.last_favorite[engine] = id;
    else
        preferences.last[engine][group] = id;
    parameter_request_cancel();
    parameter_peak_reset();
    selected_parameter_element = 0;
    page_changed = currentTime;
    menu_parameters_refresh();
}

/* Prepare the visible, sorted list and restore the requested selection. */
static void build_pages(uint16_t selected) {
    list_count =
        menu_page_list_filtered(&preferences, engine, group, view == FAVORITES || view == ORDER_FAVORITES,
                                is_editor(), gasoline_v6, advanced_pages, list);
    selection = 0;
    for (unsigned i = 0; i < list_count; ++i)
        if (parameter_pages[engine][list[i]].id == selected)
            selection = i;
    if (view != ORDER_FAVORITES)
        select_page();
}

/* Open favorites or a parameter group at its remembered page. */
static void open_pages(bool favorite) {
    view = favorite ? FAVORITES : VALUES;
    build_pages(favorite ? preferences.last_favorite[engine] : preferences.last[engine][group]);
}

/* Show an automatic result without changing the user's visibility preferences. */
void menu_show_parameter(uint8_t index) {
    if (!menu_page_supported(engine, index, gasoline_v6))
        return;
    group = parameter_pages[engine][index].group;
    view = VALUES;
    build_pages(parameter_pages[engine][index].id);
    /* Automatic result screens must not change the user's visibility preference. */
    bool listed = false;
    for (unsigned i = 0; i < list_count; ++i)
        listed |= list[i] == index;
    if (!listed && list_count < sizeof(list)) {
        selection = list_count;
        list[list_count++] = index;
        select_page();
    }
}

/* Confirm and request the selected vehicle action when its conditions are met. */
static void action_run(void) {
    MenuAction id = actions[function].id;
    const char *reason = action_unavailable(id);
    if (reason) {
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_unavailable(text, sizeof(text), reason);
        menu_notice(text);
        return;
    }
    if (id == ACTION_READ) {
        parameter_request_cancel();
        fault_reader_start(0x40);
        fault_index = 0;
        view = FAULTS;
        return;
    }
    if (id == ACTION_PEAK) {
        parameter_peak_enable(!parameter_peak_enabled());
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_checkbox(text, sizeof(text), "Peak hold", parameter_peak_enabled());
        menu_notice(text);
        return;
    }
    if (confirmed_action != id || currentTime - confirm_started > 3000) {
        confirmed_action = id;
        confirm_started = currentTime;
        menu_notice(id == ACTION_BRAKE && !chassis_state.front_brake_forced ? "Brake+launch? RES"
                    : id == ACTION_DYNO ? "Dyno+ESC? RES"
                    : id == ACTION_AWD && chassis_state.awd_sequence ? "Stop 4WD req? RES"
                    : UI_SYMBOL_WARNING " RES to confirm");
        return;
    }
    confirmed_action = 255;
    uint8_t command[2] = {C2BusID, 0};
    switch (id) {
    case ACTION_IBS:
        if (telemetry_state.current_rpm_speed <= 400) {
            menu_notice(UI_SYMBOL_WARNING " Start engine");
            return;
        }
        ibs_override_enable(!ibs_override_enabled());
        char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        ui_render_toggle(text, sizeof(text), "IBS override", ibs_override_enabled());
        menu_notice(text);
        return;
    case ACTION_CLEAR:
        diagnostics_state.clear_faults_request = 255;
        action_requested(id, true);
        menu_notice("Clear faults WAIT");
        return;
    case ACTION_STATS:
        menu_notice(statistics_reset() == 0 ? "Records cleared" : UI_SYMBOL_FAILURE " Save failed");
        return;
    case ACTION_DYNO:
        if (runtime_state.car_steady_counter < 100) {
            menu_notice(UI_SYMBOL_WARNING " Stop the car");
            return;
        }
        command[1] = C2cmdtoggleDyno;
        break;
    case ACTION_ESC:
        command[1] = C2cmdtoggleEscTc;
        break;
    case ACTION_HAS:
        command[1] = C2cmdToggleHas;
        break;
    case ACTION_LAUNCH:
        chassis_state.launch_assist_enabled = 0;
        menu_notice("Launch assist: OFF");
        return;
    case ACTION_BRAKE:
        command[1] = chassis_state.front_brake_forced ? C2cmdNormalFrontBrake : C2cmdForceFrontBrake;
        break;
    case ACTION_AWD:
        if (chassis_state.awd_sequence)
            chassis_state.awd_sequence = 0;
        else {
            if (runtime_state.car_steady_counter < 100) {
                menu_notice(UI_SYMBOL_WARNING " Stop the car");
                return;
            }
            chassis_state.awd_sequence = 4;
        }
        menu_notice("4WD requested");
        return;
    case ACTION_EXHAUST:
        comfort_state.force_q_vexhaust_valve_opened = comfort_state.force_q_vexhaust_valve_opened ? 4 : 1;
        comfort_state.chinese_exhaust_valve_request = comfort_state.chinese_valve_is_opened ? 'C' : 'O';
        menu_notice("Exhaust requested");
        return;
    default:
        return;
    }
    if (!board_uart_send(command, sizeof(command))) {
        requests[id].state = REQUEST_FAILED;
        menu_notice(UI_SYMBOL_WARNING " Queue full retry");
        return;
    }
    if (id == ACTION_HAS)
        comfort_state.has_button_press_requested = 5;
    action_requested(id, id == ACTION_DYNO ? !chassis_state.dyno_mode_enabled_on_master
                                          : id == ACTION_BRAKE ? !chassis_state.front_brake_forced : true);
    menu_notice("Request queued");
}

/* Render entry semantics and request evidence through the common presentation helpers. */
static void action_render(char *text, size_t capacity, const ActionEntry *entry) {
    MenuAction id = entry->id;
    ActionRequest *request = &requests[id];
    if (id == ACTION_CLEAR && diagnostics_state.clear_faults_request && request->state != REQUEST_TIMEOUT) {
        ui_render_pending(text, capacity, entry->name, "");
        return;
    }
    if (request->state == REQUEST_WAIT) {
        ui_render_pending(text, capacity, entry->name,
                          id == ACTION_DYNO || id == ACTION_BRAKE ? (request->target ? "ON" : "OFF") : "");
        return;
    }
    if (request->state == REQUEST_FAILED || request->state == REQUEST_TIMEOUT) {
        if (request->state == REQUEST_FAILED)
            ui_render_failure(text, capacity, "Request failed");
        else
            snprintf_(text, capacity, UI_SYMBOL_UNKNOWN " No confirmation");
        return;
    }
    const char *reason = action_unavailable(id);
    if (reason) {
        ui_render_unavailable(text, capacity, reason);
        return;
    }
    if (id == ACTION_AWD && chassis_state.awd_sequence) {
        ui_render_pending(text, capacity, "4WD req", "OFF");
    } else if (id == ACTION_EXHAUST && comfort_state.force_q_vexhaust_valve_opened) {
        ui_render_pending(text, capacity, "QV req", comfort_state.force_q_vexhaust_valve_opened == 4 ? "AUTO" : "OPEN");
    } else if ((id == ACTION_HAS || id == ACTION_CLEAR || id == ACTION_ESC) && request->state == REQUEST_SENT) {
        ui_render_value(text, capacity, entry->name, "Request sent");
    } else if (id == ACTION_DYNO) {
        ui_render_toggle(text, capacity, "Dyno", chassis_state.dyno_mode_enabled_on_master);
    } else if (id == ACTION_BRAKE) {
        /* C2 confirms its override sequence, not measured brake pressure. */
        ui_render_value(text, capacity, "Brake req", chassis_state.front_brake_forced ? "ON" : "OFF");
    } else if (entry->type == UI_ENTRY_TOGGLE) {
        ui_render_checkbox(text, capacity, entry->name, parameter_peak_enabled());
    } else if (id == ACTION_IBS) {
        ui_render_toggle(text, capacity, entry->name, ibs_override_enabled());
    } else {
        ui_render_action(text, capacity, entry->name);
    }
}

/* Present the active menu, reading, editor or feedback message. */
void menu_render(void) {
    if (!dashboard_state.baccable_dashboard_menu_visible)
        return;
    action_requests_process();
    if (save_failed) {
        menu_present(UI_SYMBOL_FAILURE " Save failed: RES");
        return;
    }
    if (notice && currentTime - notice_started < notice_duration) {
        menu_present(notice);
        return;
    }
    notice = NULL;
    if (settings_state.awd_disabler_enabled && chassis_state.awd_sequence &&
        currentTime - last_input > 1500 && currentTime % 6000 < 1000) {
        menu_present(UI_SYMBOL_WARNING " 4WD OFF request");
        return;
    }
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    unsigned position = 0, total = 0;
    switch (view) {
    case ROOT:
        snprintf_(text, sizeof(text), UI_SYMBOL_ENTER " %s", roots[root]);
        position = root + 1; total = sizeof(roots) / sizeof(roots[0]);
        break;
    case GROUPS:
        snprintf_(text, sizeof(text), UI_SYMBOL_ENTER " %s", menu_group_names[group]);
        position = group + 1; total = MENU_GROUPS;
        break;
    case FAVORITES:
    case VALUES:
        if (!list_count) {
            menu_present(view == FAVORITES ? "No favorites" : "No pages");
            return;
        }
        menu_parameters_refresh();
        dashboard_send_values();
        return;
    case FUNCTIONS: {
        if (!available(actions[function].id))
            function_move(1, false);
        action_render(text, sizeof(text), &actions[function]);
        MenuAction id = actions[function].id;
        RequestState state = requests[id].state;
        if (state != REQUEST_WAIT && state != REQUEST_FAILED && state != REQUEST_TIMEOUT &&
            !(state == REQUEST_SENT && (id == ACTION_HAS || id == ACTION_CLEAR || id == ACTION_ESC)) &&
            !(id == ACTION_AWD && chassis_state.awd_sequence) &&
            !(id == ACTION_EXHAUST && comfort_state.force_q_vexhaust_valve_opened) &&
            !(id == ACTION_CLEAR && diagnostics_state.clear_faults_request)) {
            for (unsigned i = 0; i < ACTION_COUNT; ++i)
                if (available(actions[i].id)) {
                    ++total;
                    if (i == function) position = total;
                }
        }
        break;
    }
    case SETTINGS:
        position = setting + 1; total = sizeof(settings) / sizeof(settings[0]);
        if (setting == 4)
            ui_render_value(text, sizeof(text), "Sort", preferences.alphabetical ? "A-Z" : "groups");
        else
            snprintf_(text, sizeof(text), "%s%s", setting < 4 ? UI_SYMBOL_ENTER " " : "", settings[setting]);
        break;
    case SETUP:
        dashboard_send_setup();
        return;
    case EDIT_FAVORITES:
    case EDIT_VISIBLE:
        position = selection + 1; total = list_count;
        if (!list_count) {
            menu_present("No pages");
            return;
        }
        {
            const ParameterPage *page = &parameter_pages[engine][editor_page];
            bool checked = menu_page_visible(&preferences, engine, editor_page);
            if (view == EDIT_FAVORITES) {
                checked = false;
                for (unsigned i = 0; i < MENU_FAVORITES; ++i)
                    checked |= preferences.favorites[engine][i] == page->id;
            }
            ui_render_checkbox(text, sizeof(text), page->label, checked);
        }
        break;
    case ORDER_FAVORITES:
        position = selection + 1; total = list_count;
        if (!list_count) {
            menu_present("No favorites");
            return;
        }
        snprintf_(text, sizeof(text), "%c %s", order_selected ? UI_SYMBOL_SELECTED[0] : ' ',
                  parameter_pages[engine][list[selection]].label);
        break;
    case FAULTS:
        fault_reader_text(fault_index, text, sizeof(text));
        break;
#ifdef MENU_DIAGNOSTICS
    case DIAGNOSTICS:
        menu_diagnostics_render(text, sizeof(text));
        break;
#endif
    case INFO:
        position = info + 1; total = INFO_PAGES;
#ifdef MENU_DIAGNOSTICS
        if (info == 5) {
            ui_render_action(text, sizeof(text), "IPC diag");
            break;
        }
#endif
        if (info == 0)
            ui_render_value(text, sizeof(text), "FW",
                            !strncmp(FW_VERSION, "BACCABLE ", 9) ? FW_VERSION + 9 : FW_VERSION);
        else if (info == 4)
            ui_render_toggle(text, sizeof(text), "Immobilizer", security_state.immobilizer_enabled);
        else if (info == 3)
            snprintf_(text, sizeof(text), "MY23:%s %uch",
                      settings_state.ipc_my23_is_installed ? "ON" : "OFF", DASHBOARD_MESSAGE_MAX_LENGTH);
        else {
            unsigned peer = info - 1;
            if (peer_seen[peer] && currentTime - peer_updated[peer] <= 5000)
                ui_render_value(text, sizeof(text), peer ? "BH" : "C2", peer_versions[peer]);
            else
                snprintf_(text, sizeof(text), UI_SYMBOL_UNKNOWN " %s no reply", peer ? "BH" : "C2");
        }
        break;
    }
    if (total) {
        char numbered[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        size_t prefix = ui_render_position(numbered, sizeof(numbered), position, total);
        bool dense_version = view == INFO && info <= 2 && prefix + strlen(text) >= sizeof(numbered);
        bool fits = !dense_version && ui_render_list_entry(numbered, sizeof(numbered), position, total, text);
        if (dense_version)
            snprintf_(numbered, sizeof(numbered), "%s", text);
        if (!fits && view == INFO && currentTime - info_changed < NOTICE_REQUEST_MS)
            ui_render_list_entry(numbered, sizeof(numbered), position, total,
                                 info == 0 ? "FW version" : info == 1 ? "C2 version" : "BH version");
        menu_present(numbered);
    } else
        menu_present(text);
}

/* Release the display, retrying a rejected clear until the UART accepts it. */
static void close_menu(void) {
    dashboard_state.baccable_dashboard_menu_visible = 0;
    parameter_request_cancel();
    confirmed_action = 255;
    close_pending = true;
    /* Force a fresh clear even if an earlier blank screen was already accepted. */
    previous_valid = 0;
    dashboard_clear();
}

/* Persist each changed domain independently before completing a requested exit. */
static void persist_exit(void) {
    uint8_t settings_result = settings_save();
    uint8_t preferences_result = menu_preferences_save();
    save_failed = settings_result != 0 || preferences_result != 0;
    if (save_failed) {
        menu_present(UI_SYMBOL_FAILURE " Save failed: RES");
        return;
    }
    notice = NULL;
    parameter_request_cancel();
    if (save_close) {
        fault_reader_cancel();
        close_menu();
    } else if (save_destination == FAVORITES) {
        close_pending = false;
        confirmed_action = 255;
        order_selected = 0;
        last_input = currentTime;
        open_pages(true);
        menu_render();
    } else
        view = save_destination;
}

/* Remember the destination so retries never reinterpret a navigation event. */
static void request_exit(MenuView destination, bool close) {
    save_destination = destination;
    save_close = close;
    persist_exit();
}

/* Cancel one unfinished workflow or return to its parent after persistence. */
static void back(void) {
#ifdef MENU_DIAGNOSTICS
    if (view == DIAGNOSTICS) {
        view = INFO;
        return;
    }
#endif
    if (view == FAULTS) {
        fault_reader_cancel();
        view = FUNCTIONS;
        return;
    }
    if (view == ROOT) {
        request_exit(ROOT, true);
        return;
    }
    if (view == SETUP) {
        if (setup_back())
            return;
        setup_last = setup_dashboardPageIndex;
        request_exit(SETTINGS, false);
    } else if (is_editor() || view == ORDER_FAVORITES) {
        request_exit(SETTINGS, false);
    } else if (view == SETTINGS) {
        request_exit(ROOT, false);
    } else if (view == VALUES)
        view = GROUPS;
    else
        view = ROOT;
    parameter_request_cancel();
}

/* Apply one navigation gesture to the currently visible menu. */
void menu_event(MenuEvent event) {
    if (!event)
        return;
    last_input = currentTime;
    if (engine != !!settings_state.is_diesel_enabled || gasoline_v6 != !!settings_state.gasoline_v6 ||
        advanced_pages != !!settings_state.advanced_pages)
        menu_engine_changed();
    if (!dashboard_state.baccable_dashboard_menu_visible) {
        if (event == MENU_BACK) {
            notice = NULL;
            close_pending = false;
            root = 0;
            dashboard_state.baccable_dashboard_menu_visible = 1;
            open_pages(true);
            menu_render();
        }
        return;
    }
    if (save_failed) {
        if (event == MENU_SELECT)
            persist_exit();
        else if (event == MENU_BACK) {
            save_failed = false;
            notice = NULL;
        }
        if (dashboard_state.baccable_dashboard_menu_visible)
            menu_render();
        return;
    }
    if (event != MENU_SELECT)
        confirmed_action = 255;
    notice = NULL;
    if (event == MENU_BACK) {
        back();
        menu_render();
        return;
    }
    int direction = event == MENU_PREVIOUS || event == MENU_PREVIOUS_GROUP ? -1 : 1;
    bool jump = event == MENU_NEXT_GROUP || event == MENU_PREVIOUS_GROUP;
    if (event != MENU_SELECT) {
        switch (view) {
        case ROOT:
            root = wrap(root, sizeof(roots) / sizeof(roots[0]), direction);
            break;
        case GROUPS:
            group = wrap(group, MENU_GROUPS, direction);
            break;
        case FUNCTIONS:
            function_move(direction, jump);
            break;
        case SETTINGS:
            setting = wrap(setting, sizeof(settings) / sizeof(settings[0]), direction);
            break;
        case FAULTS:
            fault_index = wrap(fault_index, fault_reader_count(), direction);
            break;
#ifdef MENU_DIAGNOSTICS
        case DIAGNOSTICS:
            menu_diagnostics_move(direction);
            break;
#endif
        case INFO:
            info = wrap(info, INFO_PAGES, direction);
            info_changed = currentTime;
            break;
        case SETUP:
            if (jump)
                setup_move_group(direction);
            else
                setup_move_page(direction);
            break;
        case FAVORITES:
        case VALUES:
        case EDIT_FAVORITES:
        case EDIT_VISIBLE:
            if (jump && view != FAVORITES) {
                group = wrap(group, MENU_GROUPS, direction);
                build_pages(is_editor() ? 0 : preferences.last[engine][group]);
            } else {
                selection = wrap(selection, list_count, direction);
                select_page();
            }
            break;
        case ORDER_FAVORITES:
            if (order_selected && list_count) {
                uint16_t id = parameter_pages[engine][list[selection]].id;
                menu_favorite_move_supported(&preferences, engine, id, direction, gasoline_v6);
                build_pages(id);
            } else
                selection = wrap(selection, list_count, direction);
            break;
        }
    } else {
        switch (view) {
        case ROOT:
            switch (root) {
            case 0:
                open_pages(true);
                break;
            case 1:
                view = GROUPS;
                break;
            case 2:
                view = FUNCTIONS;
                break;
            case 3:
                view = SETTINGS;
                break;
            case 4:
                view = INFO;
                info_changed = currentTime;
                break;
            }
            break;
        case GROUPS:
            open_pages(false);
            break;
        case FAVORITES:
        case VALUES:
            break; /* Status pages do not use SELECT as hidden backward navigation. */
        case FUNCTIONS:
            action_run();
            break;
        case SETTINGS:
            switch (setting) {
            case 0:
                view = SETUP;
                setup_dashboardPageIndex = setup_last;
                break;
            case 1:
                view = EDIT_FAVORITES;
                build_pages(0);
                break;
            case 2:
                view = EDIT_VISIBLE;
                build_pages(0);
                break;
            case 3:
                view = ORDER_FAVORITES;
                order_selected = 0;
                build_pages(0);
                break;
            case 4:
                preferences.alphabetical = !preferences.alphabetical;
                break;
            }
            break;
        case SETUP:
            setup_select_page(setup_dashboardPageIndex);
            break;
        case EDIT_FAVORITES:
            if (list_count &&
                !menu_favorite_toggle(&preferences, engine, parameter_pages[engine][editor_page].id))
                menu_notice(UI_SYMBOL_WARNING " Max 6 favorites");
            break;
        case EDIT_VISIBLE:
            if (list_count) {
                bool visible = !menu_page_visible(&preferences, engine, editor_page);
                menu_page_show(&preferences, engine, editor_page, visible);
            }
            break;
        case ORDER_FAVORITES:
            if (list_count)
                order_selected = !order_selected;
            break;
        case FAULTS:
            if (diagnostics_state.clear_faults_request)
                menu_notice(UI_SYMBOL_WARNING " Clear active");
            else {
                fault_reader_start(0x40);
                fault_index = 0;
            }
            break;
        case INFO:
#ifdef MENU_DIAGNOSTICS
            if (info == 5) {
                view = DIAGNOSTICS;
                break;
            }
#endif
            break;
#ifdef MENU_DIAGNOSTICS
        case DIAGNOSTICS:
            menu_diagnostics_select();
            break;
#endif
        }
    }
    menu_render();
}

/* Turn eligible steering-wheel button reports into menu gestures. */
void menu_button(uint8_t button, bool allowed) {
    #ifndef HIDE_DASHBOARD_MENU
    MenuEvent event = menu_input_update(&input, button, allowed, currentTime);
    bool repeat_allowed = allowed && !save_failed && dashboard_state.baccable_dashboard_menu_visible &&
                          (view == FAVORITES || view == VALUES || view == GROUPS ||
                           view == SETTINGS || view == SETUP || is_editor() ||
                           (view == ORDER_FAVORITES && !order_selected));
    if (event == MENU_NONE)
        event = menu_input_repeat(&input, repeat_allowed, currentTime);
    if (allowed && input.armed && button != 0x10)
        last_input = currentTime;
    menu_event(event);
    #else
    (void)button;
    (void)allowed;
    #endif
}

/* Refresh the display and request readings at their intended intervals. */
void menu_process(void) {
    fault_reader_process();
    action_requests_process();
    if (engine != !!settings_state.is_diesel_enabled || gasoline_v6 != !!settings_state.gasoline_v6 ||
        advanced_pages != !!settings_state.advanced_pages)
        menu_engine_changed();
    if (close_pending)
        dashboard_clear();
    if (!dashboard_state.baccable_dashboard_menu_visible)
        return;
    bool editor = view == SETTINGS || view == SETUP || is_editor() || view == ORDER_FAVORITES;
    /* Reading screens are intentionally persistent; active diagnostics get a fresh grace period. */
    if (fault_reader_busy() || diagnostics_state.clear_faults_request) {
        last_input = currentTime;
    } else if (view != FAVORITES && view != VALUES && !save_failed &&
               currentTime - last_input >= (editor ? EDITOR_IDLE_MS : MENU_IDLE_MS)) {
        if (view == SETUP) {
            setup_cancel_edit();
            setup_last = setup_dashboardPageIndex;
        }
        confirmed_action = 255;
        order_selected = 0;
        request_exit(FAVORITES, false);
        if (!save_failed)
            return;
    }
    if (menu_parameters_active() && settings_state.rotate_readings && currentTime - page_changed >= 5000) {
        selection = wrap(selection, list_count, 1);
        select_page();
    }
    if (menu_parameters_active() && !diagnostics_state.clear_faults_request &&
        currentTime - page_changed >= 150 && currentTime - last_query >= 500) {
        const ParameterPage *page = &parameter_pages[engine][dashboard_state.dashboard_page_index];
        /* One UDS transaction per interval; native values are fed by their CAN sources. */
        unsigned first = selected_parameter_element;
        for (unsigned i = 0; i < parameter_page_elements(page); ++i) {
            unsigned element = (first + i) % parameter_page_elements(page);
            if (parameter_definitions[page->parameter_ids[element]].request_id > 0xff) {
                selected_parameter_element = element;
                parameter_request_begin();
                break;
            }
        }
        selected_parameter_element = (selected_parameter_element + 1) % parameter_page_elements(page);
        last_query = currentTime;
    }
    if (currentTime - last_render >= 100) {
        last_render = currentTime;
        menu_render();
    }
}
#endif
