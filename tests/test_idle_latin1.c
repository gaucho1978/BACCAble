/* Included by test_menu.c: idle ownership and the hardware-reported single-byte alphabet. */
static void assert_production_bytes(const char *text) {
    for (; *text; ++text) {
        uint8_t byte = (uint8_t)*text;
        assert((byte >= 0x20 && byte <= 0x7e) || byte == 0xab || byte == 0xbb ||
               byte == 0xb0 || byte == 0xd7 || byte == 0xd8);
    }
}

/* Every inactive browse/configuration context returns home without releasing the overlay. */
static void test_idle_home_contexts(void) {
    const unsigned sections[] = {0, 1, 2, 3, 4};
    for (unsigned i = 0; i < sizeof(sections) / sizeof(sections[0]); ++i) {
        fresh_contract();
        menu_event(MENU_BACK);
        for (unsigned n = 0; n < sections[i]; ++n) menu_event(MENU_NEXT);
        if (sections[i]) menu_event(MENU_SELECT); /* Section zero tests ROOT itself. */
        unsigned blanks = blank_screens;
        now += sections[i] == 3 ? 60000 : 30000;
        menu_process();
        assert(menu_parameters_active() && dashboard_state.baccable_dashboard_menu_visible);
        assert(blank_screens == blanks);
        unsigned page = dashboard_state.dashboard_page_index;
        menu_event(MENU_NEXT);
        assert(dashboard_state.dashboard_page_index != page);
        menu_event(MENU_PREVIOUS);
        assert(dashboard_state.dashboard_page_index == page);
    }
    for (unsigned editor = 0; editor < 4; ++editor) {
        fresh_contract();
        to_settings();
        for (unsigned i = 0; i < editor; ++i) menu_event(MENU_NEXT);
        menu_event(MENU_SELECT);
        unsigned blanks = blank_screens;
        now += 60000;
        menu_process();
        assert(menu_parameters_active() && blank_screens == blanks);
    }
}

/* A committed setting survives timeout, but a mirror capture never happens implicitly. */
static void test_idle_commit_and_capture(void) {
    fresh_contract();
    to_settings(); menu_event(MENU_SELECT);
    setup_dashboardPageIndex = setup_page_for(3); /* LED strip is a persisted boolean. */
    assert(!settings_state.led_strip_controller_enabled);
    menu_event(MENU_SELECT);
    assert(settings_state.led_strip_controller_enabled);
    unsigned blanks = blank_screens;
    now += 60000; menu_process();
    assert(menu_parameters_active() && blank_screens == blanks);
    settings_state.led_strip_controller_enabled = 0;
    setup_load_from_flash();
    assert(settings_state.led_strip_controller_enabled);

    fresh_contract();
    to_settings(); menu_event(MENU_SELECT);
    setup_dashboardPageIndex = setup_page_for(23);
    menu_event(MENU_SELECT); menu_event(MENU_SELECT); /* Enable only. */
    menu_event(MENU_NEXT); menu_event(MENU_SELECT); /* Unconfirmed Store position. */
    assert(strstr(screen, "Adjust") && setup_in_workflow());
    unsigned before = commands;
    blanks = blank_screens;
    now += 60000; menu_process();
    assert(menu_parameters_active() && !setup_in_workflow());
    assert(commands == before && blank_screens == blanks); /* No mirror-store command. */
    assert(settings_state.park_mirror);
}

/* Check actual bytes, not UTF-8 literals, through rendering and the UART-facing screen. */
static void test_latin1_vocabulary(void) {
    assert((uint8_t)UI_GLYPH_CHECKED == 0xd8 && (uint8_t)UI_GLYPH_UNCHECKED == 0x4f);
    assert((uint8_t)UI_GLYPH_PREV == 0xab && (uint8_t)UI_GLYPH_NEXT == 0xbb);
    assert((uint8_t)UI_GLYPH_DEGREE == 0xb0 && (uint8_t)UI_GLYPH_CROSS == 0xd7);
    assert((uint8_t)UI_GLYPH_DOT == 0xb7 && (uint8_t)UI_GLYPH_PLUSMINUS == 0xb1);
    assert((uint8_t)UI_SYMBOL_FAILURE[0] == (uint8_t)UI_GLYPH_CROSS && sizeof(UI_SYMBOL_FAILURE) == 2);
    fresh_contract();
    char text[25];
    ui_render_checkbox(text, sizeof(text), "Auto rotate", true);
    assert((uint8_t)text[0] == 0xd8 && !strcmp(text + 1, " Auto rotate"));
    menu_present(text);
    assert((uint8_t)screen[0] == 0xd8 && screen[1] == ' ');
    for (unsigned i = strlen(text); i < DASHBOARD_MESSAGE_MAX_LENGTH; ++i) assert(screen[i] == ' ');
    ui_render_checkbox(text, sizeof(text), "Auto rotate", false);
    assert(!strcmp(text, "O Auto rotate"));
    for (size_t capacity = 0; capacity <= sizeof(text); ++capacity) {
        memset(text, 'X', sizeof(text));
        ui_render_signed_number(text, capacity, "Pedal trim", -10, true);
        for (size_t i = capacity; i < sizeof(text); ++i) assert(text[i] == 'X');
        if (capacity) { assert(memchr(text, 0, capacity)); assert_production_bytes(text); }
        if (capacity >= 19) assert(strstr(text, "-10"));
    }
    ui_render_number(text, 19, "Shift RPM", 3500, true);
    assert(!strcmp(text, "* Shift RPM: 3500"));
    ui_render_number(text, 25, "Shift RPM", 3500, true);
    assert((uint8_t)text[13] == 0xab && (uint8_t)text[20] == 0xbb);
    ui_render_failure(text, sizeof(text), "Read failed");
    assert((uint8_t)text[0] == 0xd7);
    ui_render_unavailable(text, sizeof(text), "Start engine");
    assert(text[0] == '!');
    ui_render_pending(text, sizeof(text), "4WD req", "OFF");
    assert(!strcmp(text, "4WD req: OFF WAIT"));
}

/* Degree insertion never consumes a value/unit cell, including dense and malformed templates. */
static void test_temperature_glyph_bounds(void) {
    const uint8_t ids[] = {42, 42, 42, 42};
    float values[] = {-20, 90, 100, 40};
    struct { char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1]; char guard; } output;
    output.guard = 'X';
    dashboard_format_values("Oil $3.0fC", values, ids, output.text);
    assert(!strcmp(output.text, "Oil -20" "\xB0" "C"));
    values[0] = NAN;
    dashboard_format_values("Oil $3.0fC", values, ids, output.text);
    assert(strstr(output.text, "--" "\xB0" "C"));
    dashboard_format_values("IC $3.0f Cool", values, ids, output.text);
    assert(!strchr(output.text, UI_GLYPH_DEGREE)); /* A label is not a Celsius unit. */
    assert(output.guard == 'X');
    for (unsigned engine = 0; engine < 2; ++engine)
        for (unsigned page = 0; page < menu_page_count(engine); ++page) {
            const ParameterPage *entry = &parameter_pages[engine][page];
            dashboard_format_values(entry->name, values, entry->parameter_ids, output.text);
            assert(output.guard == 'X');
            assert_production_bytes(output.text);
        }
}
