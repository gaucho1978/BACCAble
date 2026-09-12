#include "features/menu.h"
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
    ACTION_IBS
} MenuAction;
typedef struct {
    MenuAction id;
    uint8_t group;
    const char *name;
} ActionEntry;
static const ActionEntry actions[] = {{ACTION_EXHAUST, 0, "QV exhaust"},   {ACTION_HAS, 1, "HAS button"},
                                      {ACTION_ESC, 1, "ESC/TC"},           {ACTION_DYNO, 2, "Dyno"},
                                      {ACTION_BRAKE, 2, "Front brake"},    {ACTION_AWD, 2, "4WD"},
                                      {ACTION_READ, 3, "Read BCM faults"}, {ACTION_CLEAR, 3, "Clear faults"},
                                      {ACTION_STATS, 3, "Reset records"},  {ACTION_PEAK, 3, "Maximum hold"},
                                      {ACTION_IBS, 3, "IBS override"}};
static const char *const roots[] = {"Favorites", "Readings", "Actions", "Settings", "Information"};
static const char *const settings[] = {"Feature setup",   "Edit favorites", "Visible pages",
                                       "Order favorites", "Sort order",     "Save"};
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
static uint8_t previous_text[DASHBOARD_MESSAGE_MAX_LENGTH], previous_valid;
static uint32_t previous_sent;
static uint8_t confirmed_action = 255;
static bool retry_back;
static uint32_t confirm_started, last_input;
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
}

/* Show brief feedback about a selection, action or save result. */
void menu_notice(const char *text) {
    notice = text;
    notice_started = currentTime;
    if (dashboard_state.baccable_dashboard_menu_visible)
        menu_present(text);
}

/* Save favorites, visibility, sort order and remembered pages. */
uint8_t menu_preferences_save(void) {
    uint8_t data[MENU_PREFS_SIZE];
    menu_preferences_encode(&preferences, data);
    return flash_record_save(VISIBILITY_RECORD, 0x104, data, sizeof(data)) ? 0 : 255;
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
    view = FAVORITES;
    root = 0;
    group = 1;
    function = 0;
    setting = 0;
    info = 0;
    setup_last = 0;
    notice = NULL;
    confirmed_action = 255;
    previous_valid = 0;
    last_query = 0;
    last_render = 0;
    order_selected = 0;
    retry_back = false;
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
    if (!available(id)) {
        menu_notice("! Unavailable");
        return;
    }
    if (id == ACTION_READ) {
        if (diagnostics_state.clear_faults_request) {
            menu_notice("Clear in progress");
            return;
        }
        parameter_request_cancel();
        fault_reader_start(0x40);
        fault_index = 0;
        view = FAULTS;
        return;
    }
    if (id == ACTION_PEAK) {
        parameter_peak_enable(!parameter_peak_enabled());
        menu_notice(parameter_peak_enabled() ? "+ Maximum hold" : "- Maximum hold");
        return;
    }
    if (confirmed_action != id || currentTime - confirm_started > 3000) {
        confirmed_action = id;
        confirm_started = currentTime;
        menu_notice("! RES to confirm");
        return;
    }
    confirmed_action = 255;
    uint8_t command[2] = {C2BusID, 0};
    switch (id) {
    case ACTION_IBS:
        if (telemetry_state.current_rpm_speed <= 400) {
            menu_notice("! Start engine");
            return;
        }
        ibs_override_enable(!ibs_override_enabled());
        menu_notice(ibs_override_enabled() ? "+ IBS override" : "- IBS override");
        return;
    case ACTION_CLEAR:
        diagnostics_state.clear_faults_request = 255;
        menu_notice("Clear requested");
        return;
    case ACTION_STATS:
        menu_notice(statistics_reset() == 0 ? "Records cleared" : "! Save failed");
        return;
    case ACTION_DYNO:
        if (runtime_state.car_steady_counter < 100) {
            menu_notice("! Stop the car");
            return;
        }
        command[1] = C2cmdtoggleDyno;
        break;
    case ACTION_ESC:
        command[1] = C2cmdtoggleEscTc;
        break;
    case ACTION_HAS:
        comfort_state.has_button_press_requested = 5;
        command[1] = C2cmdToggleHas;
        break;
    case ACTION_BRAKE:
        if (chassis_state.front_brake_forced) {
            if (chassis_state.launch_assist_enabled) {
                chassis_state.launch_assist_enabled = 0;
                menu_notice("- Launch assist");
                return;
            }
            command[1] = C2cmdNormalFrontBrake;
        } else {
            if (telemetry_state.current_speed_km_h != 0) {
                menu_notice("! Stop the car");
                return;
            }
            if (!chassis_state.dyno_mode_enabled_on_master) {
                menu_notice("! Enable Dyno");
                return;
            }
            command[1] = C2cmdForceFrontBrake;
        }
        break;
    case ACTION_AWD:
        if (chassis_state.awd_sequence)
            chassis_state.awd_sequence = 0;
        else {
            if (runtime_state.car_steady_counter < 100) {
                menu_notice("! Stop the car");
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
    menu_notice(board_uart_send(command, sizeof(command)) ? "Command queued" : "! Queue full retry");
}

/* Describe the selected action's current state or requested change. */
static const char *action_status(MenuAction id) {
    switch (id) {
    case ACTION_IBS:
        return ibs_override_enabled() ? "+" : "-";
    case ACTION_PEAK:
        return parameter_peak_enabled() ? "+" : "-";
    case ACTION_CLEAR:
        return diagnostics_state.clear_faults_request ? "WAIT" : "RES";
    case ACTION_DYNO:
        return chassis_state.dyno_mode_enabled_on_master ? "+" : "-";
    case ACTION_BRAKE:
        return chassis_state.front_brake_forced ? "+" : "-";
    case ACTION_AWD:
        return chassis_state.awd_sequence ? "4WD OFF requested" : "4WD RES";
    case ACTION_EXHAUST:
        /* The sequence tracks our request, not measured valve position. */
        if (comfort_state.force_q_vexhaust_valve_opened == 4)
            return "QV AUTO requested";
        return comfort_state.force_q_vexhaust_valve_opened ? "QV OPEN requested" : "QV exhaust RES";
    default:
        return "RES";
    }
}

/* Present the active menu, reading, editor or feedback message. */
void menu_render(void) {
    if (!dashboard_state.baccable_dashboard_menu_visible)
        return;
    if (notice && currentTime - notice_started < 1200) {
        menu_present(notice);
        return;
    }
    notice = NULL;
    if (settings_state.awd_disabler_enabled && chassis_state.awd_sequence &&
        currentTime - last_input > 1500 && currentTime % 6000 < 1000) {
        menu_present("! 4WD OFF request");
        return;
    }
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    switch (view) {
    case ROOT:
        snprintf_(text, sizeof(text), "> %u/5 %s", root + 1, roots[root]);
        break;
    case GROUPS:
        snprintf_(text, sizeof(text), "> %u/7 %s", group + 1, menu_group_names[group]);
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
    case FUNCTIONS:
        if (!available(actions[function].id))
            function_move(1, false);
        {
            const char *status = action_status(actions[function].id);
            if (actions[function].id == ACTION_BRAKE && chassis_state.front_brake_forced &&
                chassis_state.launch_assist_enabled)
                snprintf_(text, sizeof(text), "+ Brake: Launch");
            else if (actions[function].id == ACTION_AWD || actions[function].id == ACTION_EXHAUST)
                snprintf_(text, sizeof(text), "%s", status);
            else if (status[0] == '+' || status[0] == '-')
                snprintf_(text, sizeof(text), "%c %s", status[0], actions[function].name);
            else if (actions[function].id == ACTION_READ)
                snprintf_(text, sizeof(text), "> %s", actions[function].name);
            else
                snprintf_(text, sizeof(text), "%s %s", actions[function].name, status);
        }
        break;
    case SETTINGS:
        if (setting == 4)
            snprintf_(text, sizeof(text), "* Sort: %s", preferences.alphabetical ? "A-Z" : "groups");
        else
            snprintf_(text, sizeof(text), "%s%s", setting < 4 ? "> " : "", settings[setting]);
        break;
    case SETUP:
        dashboard_send_setup();
        return;
    case EDIT_FAVORITES:
    case EDIT_VISIBLE:
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
            snprintf_(text, sizeof(text), "%c %s", checked ? '+' : '-', page->label);
        }
        break;
    case ORDER_FAVORITES:
        if (!list_count) {
            menu_present("No favorites");
            return;
        }
        snprintf_(text, sizeof(text), "%c %s", order_selected ? '*' : ' ',
                  parameter_pages[engine][list[selection]].label);
        break;
    case FAULTS:
        fault_reader_text(fault_index, text, sizeof(text));
        break;
    case INFO:
        if (info == 0)
            snprintf_(text, sizeof(text), "%s", FW_VERSION);
        else if (info == 4)
            snprintf_(text, sizeof(text), "%c Immobilizer", security_state.immobilizer_enabled ? '+' : '-');
        else if (info == 3)
            snprintf_(text, sizeof(text), "MY23:%s %u chars",
                      settings_state.ipc_my23_is_installed ? "ON" : "OFF", DASHBOARD_MESSAGE_MAX_LENGTH);
        else {
            unsigned peer = info - 1;
            if (peer_seen[peer] && currentTime - peer_updated[peer] <= 5000)
                snprintf_(text, sizeof(text), "%s %s", peer ? "BH" : "C2", peer_versions[peer]);
            else
                snprintf_(text, sizeof(text), "? %s no reply", peer ? "BH" : "C2");
        }
        break;
    }
    menu_present(text);
}

/* Save device settings and menu preferences, reporting any failure. */
static bool save_all(void) {
    if (settings_save() != 0 || menu_preferences_save() != 0) {
        menu_notice("! Save failed: RES");
        return false;
    }
    menu_notice("Saved");
    return true;
}

/* Return to the parent menu and save edits before leaving their editor. */
static void back(void) {
    if (view == FAULTS) {
        fault_reader_cancel();
        view = FUNCTIONS;
        return;
    }
    if (view == ROOT) {
        if (!save_all()) {
            retry_back = true;
            return;
        }
        dashboard_state.baccable_dashboard_menu_visible = 0;
        parameter_request_cancel();
        dashboard_clear();
        return;
    }
    if (view == SETUP) {
        setup_last = setup_dashboardPageIndex;
        if (!save_all()) {
            retry_back = true;
            return;
        }
        view = SETTINGS;
    } else if (is_editor() || view == ORDER_FAVORITES) {
        if (menu_preferences_save() != 0) {
            retry_back = true;
            menu_notice("! Save failed: RES");
            return;
        }
        view = SETTINGS;
        menu_notice("Saved");
    } else if (view == VALUES)
        view = GROUPS;
    else {
        view = ROOT;
    }
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
            root = 0;
            dashboard_state.baccable_dashboard_menu_visible = 1;
            open_pages(true);
            menu_render();
        }
        return;
    }
    if (event == MENU_SELECT && retry_back)
        event = MENU_BACK;
    retry_back = false;
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
            root = wrap(root, 5, direction);
            break;
        case GROUPS:
            group = wrap(group, MENU_GROUPS, direction);
            break;
        case FUNCTIONS:
            function_move(direction, jump);
            break;
        case SETTINGS:
            setting = wrap(setting, 6, direction);
            break;
        case FAULTS:
            fault_index = wrap(fault_index, fault_reader_count(), direction);
            break;
        case INFO:
            info = wrap(info, 5, direction);
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
                break;
            }
            break;
        case GROUPS:
            open_pages(false);
            break;
        case FAVORITES:
        case VALUES:
            view = ROOT;
            parameter_request_cancel();
            break;
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
            case 5:
                save_all();
                break;
            }
            break;
        case SETUP:
            if (setup_dashboardPageIndex == 0) {
                if (save_all())
                    view = SETTINGS;
            } else
                setup_select_page(setup_dashboardPageIndex);
            break;
        case EDIT_FAVORITES:
            if (list_count &&
                !menu_favorite_toggle(&preferences, engine, parameter_pages[engine][editor_page].id))
                menu_notice("! Max 6 favorites");
            break;
        case EDIT_VISIBLE:
            if (list_count) {
                bool visible = !menu_page_visible(&preferences, engine, editor_page);
                menu_page_show(&preferences, engine, editor_page, visible);
            }
            break;
        case ORDER_FAVORITES:
            order_selected = !order_selected;
            break;
        case FAULTS:
            fault_reader_start(0x40);
            fault_index = 0;
            break;
        case INFO:
            view = ROOT;
            break;
        }
    }
    menu_render();
}

/* Turn eligible steering-wheel button reports into menu gestures. */
void menu_button(uint8_t button, bool allowed) {
    #ifndef HIDE_DASHBOARD_MENU
    menu_event(menu_input_update(&input, button, allowed, currentTime));
    #else
    (void)button;
    (void)allowed;
    #endif
}

/* Refresh the display and request readings at their intended intervals. */
void menu_process(void) {
    fault_reader_process();
    if (engine != !!settings_state.is_diesel_enabled || gasoline_v6 != !!settings_state.gasoline_v6 ||
        advanced_pages != !!settings_state.advanced_pages)
        menu_engine_changed();
    if (!dashboard_state.baccable_dashboard_menu_visible)
        return;
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
