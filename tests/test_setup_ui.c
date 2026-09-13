/* Included by test_menu.c to reuse the board transport and settings fixture. */
static uint8_t setup_page_for(uint8_t slot) {
    const SetupParam *param = setup_find_by_flash_index(slot);
    assert(param && param->menu_text);
    setup_cancel_edit();
    for (uint8_t page = 0; page < 40; ++page) {
        setup_render_page(page);
        char expected[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
        if (param->entry_type == UI_ENTRY_TOGGLE)
            ui_render_toggle(expected, sizeof(expected), param->menu_text, *(uint8_t *)param->value != 0);
        else
            snprintf_(expected, sizeof(expected), "%s", param->menu_text);
        if (!memcmp(dashboard_setup_screen, expected, strlen(expected))) {
            setup_dashboardPageIndex = page;
            return page;
        }
    }
    assert(!"setup page missing");
    return 0;
}
static void test_setup_ui(void) {
    const uint8_t enum_slots[] = {16, 20, 24, 25, 26};
    const uint8_t enum_sizes[] = {3, 9, 3, 3, 3};
    for (unsigned entry = 0; entry < sizeof(enum_slots); ++entry) {
        const SetupParam *param = setup_find_by_flash_index(enum_slots[entry]);
        assert(param->entry_type == UI_ENTRY_ENUM);
        *(uint8_t *)param->value = 0;
        settings_state.gasoline_v6 = 0;
        for (unsigned step = 0; step < enum_sizes[entry]; ++step) {
            param->action();
            assert(*(uint8_t *)param->value <= param->max_value);
        }
        assert(*(uint8_t *)param->value == 0);
    }
    const SetupParam *toggle = setup_find_by_flash_index(33);
    settings_state.rotate_readings = 0;
    uint8_t toggle_page = setup_page_for(33);
    assert(toggle->entry_type == UI_ENTRY_TOGGLE);
    setup_select_page(toggle_page);
    assert(settings_state.rotate_readings);
    setup_select_page(toggle_page);
    assert(!settings_state.rotate_readings);
    setup_cancel_edit();
    settings_state.shift_threshold = 3500;
    uint8_t page = setup_page_for(5);
    setup_select_page(page);
    assert(setup_in_workflow());
    setup_move_page(1);
    assert(settings_state.shift_threshold == 3500);
    setup_select_page(page);
    assert(settings_state.shift_threshold == 3750);
    setup_select_page(page);
    setup_move_group(-1);
    assert(setup_back());
    assert(settings_state.shift_threshold == 3750);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_select_page(page);
    assert(settings_state.shift_threshold == 6000);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(-1);
    setup_select_page(page);
    assert(settings_state.shift_threshold == 1500);

    settings_state.launch_torque_threshold = 25;
    page = setup_page_for(18);
    setup_select_page(page);
    setup_move_page(-1);
    setup_select_page(page);
    assert(settings_state.launch_torque_threshold == 25);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_select_page(page);
    assert(settings_state.launch_torque_threshold == 600);

    settings_state.pedal_map_power = -10;
    page = setup_page_for(29);
    setup_select_page(page);
    setup_move_page(1);
    setup_select_page(page);
    assert(settings_state.pedal_map_power == -8);
    setup_select_page(page);
    for (unsigned i = 0; i < 30; ++i) setup_move_page(1);
    setup_select_page(page);
    assert(settings_state.pedal_map_power == 10);
    setup_select_page(page);
    setup_move_page(-1);
    setup_cancel_edit();
    assert(settings_state.pedal_map_power == 10);
    assert(setup_read_flash_value(29, (uint8_t)-10) == (uint8_t)-10);
    assert(setup_read_flash_value(5, 0) == setup_find_by_flash_index(5)->default_value);

    settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    page = setup_page_for(34);
    setup_select_page(page);
    assert(settings_state.usb_sniffer && !settings_state.usb_elm327);
    setup_select_page(page);
    assert(!settings_state.usb_sniffer);
#ifdef ACT_AS_ELM327
    assert(settings_state.usb_elm327);
    setup_select_page(page);
#endif
    assert(!settings_state.usb_sniffer && !settings_state.usb_elm327);
    assert(setup_find_by_flash_index(35)->menu_text == NULL);

    settings_state.park_mirror = 0;
    page = setup_page_for(23);
    unsigned before = commands;
    setup_select_page(page);
    assert(commands == before && !settings_state.park_mirror);
    setup_move_page(1);
    setup_select_page(page); /* Capture unavailable while disabled. */
    assert(commands == before);
    setup_move_page(-1);
    setup_select_page(page); /* Explicit enable does not capture. */
    assert(settings_state.park_mirror && commands == before + 1);
    assert(last_command == BHcmdFunctParkMirrorEnabled);
    setup_move_page(1);
    setup_select_page(page);
    assert(commands == before + 1);
    assert(setup_back()); /* Cancel confirmation. */
    assert(commands == before + 1);
    setup_select_page(page);
    uart_busy = true;
    setup_select_page(page);
    assert(commands == before + 1);
    uart_busy = false;
    setup_select_page(page);
    assert(commands == before + 2);
    assert(last_command == BHcmdFunctParkMirrorStoreCurPos);
    setup_render_page(page);
    assert(!memcmp(dashboard_setup_screen, "Store: queued", 13));
    assert(setup_back());
    assert(setup_back());
    assert(!setup_in_workflow());
}
