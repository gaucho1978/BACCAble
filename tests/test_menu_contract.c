/* Included by test_menu.c: complete navigation and position contracts at both widths. */
static void expect_position(unsigned current, unsigned total) {
    unsigned actual = 0, count = 0;
    assert(sscanf(screen, "%u/%u", &actual, &count) == 2);
    if (actual != current || count != total)
        fprintf(stderr, "Expected %u/%u, got [%s]\n", current, total, screen);
    assert(actual == current && count == total);
}

/* Begin each contract with no outstanding vehicle operation or permission. */
static void fresh_contract(void) {
    memset(&settings_state, 0, sizeof(settings_state));
    memset(&chassis_state, 0, sizeof(chassis_state));
    memset(&diagnostics_state, 0, sizeof(diagnostics_state));
    comfort_state.force_q_vexhaust_valve_opened = 0;
    comfort_state.has_button_press_requested = 0;
    uart_busy = false;
    fault_reader_cancel();
    fresh_menu();
}

/* Every section returns exactly one level, while status SELECT stays on the same page. */
static void test_navigation_contract(void) {
    for (unsigned section = 0; section < 5; ++section) {
        fresh_contract();
        char original[sizeof(screen)];
        memcpy(original, screen, sizeof(screen));
        unsigned page = dashboard_state.dashboard_page_index;
        menu_event(MENU_SELECT);
        assert(menu_parameters_active() && dashboard_state.dashboard_page_index == page);
        assert(!strcmp(original, screen));
        menu_event(MENU_BACK);
        expect_position(1, 5);
        for (unsigned i = 0; i < section; ++i) menu_event(MENU_NEXT);
        expect_position(section + 1, 5);
        menu_event(MENU_SELECT);
        if (section == 1) {
            expect_position(2, MENU_GROUPS);
            menu_event(MENU_SELECT);
            assert(menu_parameters_active());
            page = dashboard_state.dashboard_page_index;
            menu_event(MENU_SELECT);
            assert(menu_parameters_active() && dashboard_state.dashboard_page_index == page);
            menu_event(MENU_BACK);
            expect_position(2, MENU_GROUPS);
        } else if (section == 3) {
            for (unsigned editor = 0; editor < 4; ++editor) {
                expect_position(editor + 1, 5);
                menu_event(MENU_SELECT);
                if (editor == 0) {
                    unsigned current, count;
                    assert(setup_list_position(&current, &count));
                    expect_position(current, count);
                }
                menu_event(MENU_BACK);
                expect_position(editor + 1, 5);
                menu_event(MENU_NEXT);
            }
        } else if (section == 4) {
            for (unsigned i = 0; i < 5; ++i) {
                memcpy(original, screen, sizeof(screen));
                menu_event(MENU_SELECT);
                assert(!strcmp(original, screen));
                menu_event(MENU_NEXT);
            }
#ifdef MENU_DIAGNOSTICS
            expect_position(6, 6);
            menu_event(MENU_SELECT);
            assert(!strncmp(screen, "20-27 ", 6));
            menu_event(MENU_BACK);
            expect_position(6, 6);
#endif
        }
        menu_event(MENU_BACK);
        expect_position(section + 1, 5);
        menu_event(MENU_BACK);
        assert(!dashboard_state.baccable_dashboard_menu_visible);
    }
}

/* Real visible peers determine totals, including temporarily unavailable actions. */
static void test_visible_position_contract(void) {
    fresh_contract();
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
    for (unsigned i = 0; i < 3; ++i) {
        expect_position(i + 1, 3);
        menu_event(MENU_NEXT);
    }
    expect_position(1, 3);
    settings_state.has_function_enabled = 1;
    menu_event(MENU_NEXT); menu_event(MENU_NEXT); /* IBS remains browsable even with engine off. */
    expect_position(4, 4);
    assert(strstr(screen, "Start engine"));
    menu_event(MENU_NEXT);
    expect_position(1, 4);

    fresh_contract();
    to_settings();
    menu_event(MENU_SELECT);
    unsigned visible = 0, current, count;
    for (unsigned i = 0; i < setup_params_count; ++i)
        visible += setup_params[i].menu_text != NULL;
    for (unsigned i = 0; i < visible; ++i) {
        assert(setup_list_position(&current, &count));
        assert(count == visible);
        expect_position(i + 1, visible);
        assert(!strstr(screen, "Save"));
        menu_event(MENU_NEXT);
    }
    expect_position(1, visible);

    setup_dashboardPageIndex = setup_page_for(5);
    menu_event(MENU_SELECT);
    assert(!setup_list_position(&current, &count));
    assert(screen[0] == '*');
    menu_event(MENU_BACK);
    assert(setup_list_position(&current, &count));
    expect_position(current, count);
    setup_dashboardPageIndex = setup_page_for(23);
    menu_event(MENU_SELECT);
    expect_position(1, 3);
    menu_event(MENU_SELECT); /* Enable without capture. */
    menu_event(MENU_NEXT);
    expect_position(2, 3);
    menu_event(MENU_SELECT);
    assert(!setup_list_position(&current, &count));
    assert(strstr(screen, "Adjust"));
    unsigned before = commands;
    menu_event(MENU_BACK);
    expect_position(2, 3);
    assert(commands == before);
    menu_event(MENU_BACK);
    assert(setup_list_position(&current, &count));
    expect_position(current, count);
    menu_event(MENU_BACK);
    expect_position(1, 5);
}

/* Every supported template retains all its original output, including unavailable readings. */
static void test_reading_position_integrity(void) {
    for (unsigned engine = 0; engine < 2; ++engine) {
        fresh_contract();
        settings_state.is_diesel_enabled = engine;
        settings_state.gasoline_v6 = !engine;
        settings_state.advanced_pages = 1;
        menu_engine_changed();
        for (unsigned page = 0; page < menu_page_count(engine); ++page) {
            if (!menu_page_supported(engine, page, !engine)) continue;
            for (unsigned missing = 0; missing < 2; ++missing) {
                menu_show_parameter(page);
                float values[4] = {1, 2, 3, 4};
                if (missing) for (unsigned i = 0; i < 4; ++i) values[i] = NAN;
                char original[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
                const ParameterPage *entry = &parameter_pages[engine][page];
                dashboard_format_values(entry->name, values, entry->parameter_ids, original);
                menu_present_reading(original);
                unsigned current, total;
                assert(sscanf(screen, "%u/%u", &current, &total) == 2);
                assert(current && current <= total);
                char prefix[16];
                size_t used = ui_render_position(prefix, sizeof(prefix), current, total);
                now += 1200;
                menu_present_reading(original);
                const char *reading = used + strlen(original) <= DASHBOARD_MESSAGE_MAX_LENGTH ? screen + used : screen;
                assert(!strncmp(reading, original, strlen(original)));
                for (size_t i = strlen(original); reading + i < screen + DASHBOARD_MESSAGE_MAX_LENGTH; ++i)
                    assert(reading[i] == ' ');
            }
        }
    }
}

/* Bounded counters retain complete values and condition text, including the narrow display. */
static void test_position_renderer_bounds(void) {
    char text[32];
    for (size_t capacity = 0; capacity <= sizeof(text); ++capacity) {
        memset(text, 'X', sizeof(text));
        ui_render_list_entry(text, capacity, 32, 64, "Long setting name: -10");
        for (size_t i = capacity; i < sizeof(text); ++i) assert(text[i] == 'X');
        if (capacity) assert(memchr(text, 0, capacity));
        if (capacity >= 19) assert(strstr(text, ": -10"));
    }
    ui_render_list_entry(text, 19, 12, 12, "! Release brk");
    assert(strstr(text, "Release brk"));
    ui_render_list_entry(text, 19, 1, 5, "Features >");
    assert(!strcmp(text, "1/5 > Features"));
    ui_render_list_entry(text, 19, 0, 0, "No pages");
    assert(!strcmp(text, "No pages"));
}

/* Profile filtering changes visible totals without changing favorite identities. */
static void test_filtered_editor_positions(void) {
    for (unsigned v6 = 0; v6 < 2; ++v6) {
        fresh_contract();
        MenuPreferences prefs;
        menu_preferences_default(&prefs);
        assert(menu_favorite_toggle(&prefs, 0, 0x2f));
        assert(menu_favorite_toggle(&prefs, 0, 0x22));
        menu_preferences_encode(&prefs, saved);
        have_saved = true;
        settings_state.gasoline_v6 = v6;
        dashboard_state.baccable_dashboard_menu_visible = 0;
        menu_init();
        menu_event(MENU_BACK);
        unsigned favorite_count = v6 ? 6 : 5;
        expect_position(1, favorite_count);
        for (unsigned i = 0; i < favorite_count; ++i) {
            expect_position(i + 1, favorite_count);
            menu_event(MENU_NEXT);
        }
        to_settings();
        menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Edit favorites. */
        menu_event(MENU_PREVIOUS_GROUP); /* All pages, not only the remembered Engine group. */
        expect_position(1, gasoline_page_count - 2);
        menu_event(MENU_BACK); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        expect_position(1, gasoline_page_count - 2); /* Visible pages. */
        menu_event(MENU_BACK); menu_event(MENU_NEXT); menu_event(MENU_SELECT);
        expect_position(1, favorite_count);
        menu_event(MENU_SELECT);
        assert(strstr(screen, "*"));
        menu_event(MENU_NEXT);
        expect_position(2, favorite_count);
        menu_event(MENU_BACK);
        expect_position(4, 5);
        assert(menu_preferences_save() == 0);
        assert(menu_preferences_decode(&prefs, saved));
        bool retained = false;
        for (unsigned i = 0; i < MENU_FAVORITES; ++i) retained |= prefs.favorites[0][i] == 0x2f;
        assert(retained); /* Hidden I4-incompatible favorite remains stored. */
    }
}

/* Fault workflows and long peer versions retain content instead of accepting counter truncation. */
static void test_workflow_and_version_positions(void) {
    open_test_action("BCM faults");
    menu_event(MENU_SELECT);
    assert(fault_reader_busy() && !strncmp(screen, "Reading", 7));
    fault_reader_cancel();
    diagnostics_state.clear_faults_request = 1;
    menu_event(MENU_SELECT);
    assert(!fault_reader_busy() && strstr(screen, "Clear active"));
    diagnostics_state.clear_faults_request = 0;
    menu_event(MENU_BACK);
    assert(!fault_reader_busy() && strstr(screen, "BCM faults"));
    expect_position(8, 12);
    menu_event(MENU_BACK);
    expect_position(3, 5);

    open_test_action("Dyno:");
    menu_event(MENU_SELECT); menu_event(MENU_SELECT);
    now += 1300;
    menu_render();
    assert(strstr(screen, "WAIT"));
    chassis_state.dyno_mode_enabled_on_master = 1;
    menu_action_reply(C1cmdDynoActive);
    menu_render();
    expect_position(4, 12); /* A resolved action is a peer again. */

    fresh_contract();
    uint8_t version[DASHBOARD_MESSAGE_MAX_LENGTH] = {0};
    memcpy(version, "beta-1234567890", 15);
    menu_peer_status(0, version);
    menu_event(MENU_BACK);
    for (unsigned i = 0; i < 4; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT); menu_event(MENU_NEXT);
#ifdef MENU_DIAGNOSTICS
    expect_position(2, 6);
#else
    expect_position(2, 5);
#endif
    now += 1200;
    menu_render();
    assert(strstr(screen, "beta-1234567890"));
    now += 5001;
    menu_render();
    assert(strstr(screen, "no reply") && !strstr(screen, "beta-123"));
}
