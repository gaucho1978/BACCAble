/* Included by test_menu.c: observable interaction contracts shared by both display widths. */
static void open_test_action(const char *label) {
    memset(&settings_state, 0, sizeof(settings_state));
    memset(&chassis_state, 0, sizeof(chassis_state));
    memset(&diagnostics_state, 0, sizeof(diagnostics_state));
    comfort_state.has_button_press_requested = 0;
    telemetry_state.current_rpm_speed = 1000;
    telemetry_state.current_speed_km_h = 0;
    runtime_state.car_steady_counter = 100;
    settings_state.dyno_mode_master_enabled = 1;
    settings_state.front_brake_forcer_master = 1;
    settings_state.awd_disabler_enabled = 1;
    settings_state.qv_exhaust_flap_function_enabled = 1;
    settings_state.clear_faults_enabled = settings_state.read_faults_enabled = 1;
    settings_state.has_function_enabled = settings_state.esc_tc_customizator_enabled = 1;
    comfort_state.force_q_vexhaust_valve_opened = 0;
    if (!strcmp(label, "Brake req"))
        chassis_state.dyno_mode_enabled_on_master = 1;
    uart_busy = false;
    fault_reader_cancel();
    fresh_menu();
    menu_event(MENU_BACK);
    menu_event(MENU_NEXT);
    menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    const char *search = !strcmp(label, "IBS SOC override") ? "IBS" : !strcmp(label, "Brake req") ? "Brake" : label;
    for (unsigned i = 0; i < 20 && !strstr(screen, search); ++i)
        menu_event(MENU_NEXT);
    assert(strstr(screen, search));
}

static void test_shared_renderers(void) {
    char text[25];
    for (size_t size = 19; size <= 25; size += 6) {
        ui_render_toggle(text, size, "Auto rotate", false);
        assert(!strcmp(text, "Auto rotate: OFF"));
        ui_render_toggle(text, size, "Auto rotate", true);
        assert(!strcmp(text, "Auto rotate: ON"));
        ui_render_value(text, size, "ACC resume", "RES");
        assert(!strcmp(text, "ACC resume: RES"));
        ui_render_signed_number(text, size, "Pedal trim", 4, false);
        assert(!strcmp(text, "Pedal trim: +4"));
        ui_render_number(text, size, "Shift RPM", 3500, true);
        assert(!strcmp(text, size == 19 ? "* Shift RPM: 3500" : "* Shift RPM: " "\xAB" " 3500 " "\xBB"));
        ui_render_action(text, size, "Read faults");
        assert(!strcmp(text, "> Read faults"));
        ui_render_pending(text, size, "4WD req", "OFF");
        assert(!strcmp(text, "4WD req: OFF WAIT"));
        ui_render_unavailable(text, size, "Start engine");
        assert(!strcmp(text, "! Start engine"));
    }
    for (size_t size = 0; size < sizeof(text); ++size) {
        memset(text, 'X', sizeof(text));
        ui_render_toggle(text, size, "A very long label", false);
        for (size_t i = size; i < sizeof(text); ++i)
            assert(text[i] == 'X');
        if (size) assert(memchr(text, 0, size));
    }
}

static void test_pending_action_feedback(void) {
    open_test_action("Dyno:");
    unsigned before = commands;
    menu_event(MENU_SELECT);
    assert(strstr(screen, "ESC") && commands == before);
    menu_event(MENU_SELECT);
    assert(last_command == C2cmdtoggleDyno && commands == before + 1);
    now += 1300;
    menu_render();
    assert(strstr(screen, "ON WAIT"));
    chassis_state.dyno_mode_enabled_on_master = 1;
    menu_action_reply(C1cmdDynoActive);
    menu_render();
    assert(strstr(screen, "Dyno: ON") && !strstr(screen, "WAIT"));
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    now += 10000;
    menu_render();
    assert(strstr(screen, "No confirmation"));
    assert(chassis_state.dyno_mode_enabled_on_master); /* Timeout is not a fake physical OFF. */
    menu_event(MENU_SELECT);
    uart_busy = true;
    before = commands;
    menu_event(MENU_SELECT);
    assert(commands == before);
    uart_busy = false;
    menu_render();
    assert(strstr(screen, "Queue full"));

    open_test_action("HAS button");
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    now += 1300;
    menu_render();
    assert(strstr(screen, "WAIT"));
    comfort_state.has_button_press_requested = 0;
    menu_render();
    assert(strstr(screen, "Request sent")); /* Only local injection completion, not HAS engagement. */
}

static void test_conditional_actions(void) {
    open_test_action("IBS SOC override");
    ibs_override_enable(false);
    telemetry_state.current_rpm_speed = 400;
    menu_render();
    assert(strstr(screen, "Start engine"));
    menu_event(MENU_SELECT);
    assert(!ibs_override_enabled());
    telemetry_state.current_rpm_speed = 401;
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    assert(ibs_override_enabled());
    ibs_override_enable(false);

    open_test_action("Brake req");
    menu_event(MENU_SELECT);
    assert(strstr(screen, "launch"));
    menu_event(MENU_SELECT);
    assert(last_command == C2cmdForceFrontBrake);
    chassis_state.front_brake_forced = chassis_state.launch_assist_enabled = 1;
    menu_action_reply(C1cmdForceFrontBrake);
    now += 1300;
    menu_render();
    assert(strstr(screen, "End launch"));
    unsigned before = commands;
    menu_event(MENU_SELECT);
    assert(chassis_state.launch_assist_enabled && commands == before);
    menu_event(MENU_NEXT); /* The explicitly named Release launch action. */
    assert(strstr(screen, "End launch"));
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    assert(!chassis_state.launch_assist_enabled && chassis_state.front_brake_forced);
    menu_event(MENU_PREVIOUS);
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    assert(last_command == C2cmdNormalFrontBrake);
    chassis_state.front_brake_forced = 0;
    menu_action_reply(C1cmdNormalFrontBrake);
    now += 1300;
    menu_render();
    assert(strstr(screen, "Brake") && strstr(screen, ": OFF"));
}

static void test_fault_action_exclusion(void) {
    open_test_action("BCM faults");
    diagnostics_state.clear_faults_request = 1;
    menu_render();
    assert(strstr(screen, "Clear active"));
    menu_event(MENU_SELECT);
    assert(!fault_reader_busy());
    diagnostics_state.clear_faults_request = 0;
    menu_event(MENU_SELECT);
    assert(fault_reader_busy());
    menu_event(MENU_BACK);
    assert(!fault_reader_busy());
    menu_event(MENU_NEXT); /* Clear faults */
    fault_reader_start(0x40);
    menu_render();
    assert(strstr(screen, "Read active"));
    menu_event(MENU_SELECT);
    assert(!diagnostics_state.clear_faults_request);
    fault_reader_cancel();
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    assert(diagnostics_state.clear_faults_request == 255);
    diagnostics_state.clear_faults_request = 0;
    now += 1300;
    menu_render();
    assert(strstr(screen, "Request sent"));
}

static void test_numeric_menu_flow(void) {
    memset(&settings_state, 0, sizeof(settings_state));
    fresh_menu();
    to_settings();
    menu_event(MENU_SELECT);
    setup_dashboardPageIndex = setup_page_for(5);
    settings_state.shift_threshold = 3500;
    assert(settings_save() == 0 && menu_preferences_save() == 0);
    settings_writes = preference_writes = usb_applies = 0;
    menu_event(MENU_SELECT);
    assert(strstr(screen, "* Shift RPM"));
    menu_event(MENU_NEXT);
    assert(strstr(screen, "3750") && settings_state.shift_threshold == 3500);
    menu_event(MENU_BACK);
    assert(!setup_in_workflow() && settings_state.shift_threshold == 3500);
    assert(settings_writes == 0 && preference_writes == 0 && usb_applies == 0);
    menu_event(MENU_SELECT);
    menu_event(MENU_PREVIOUS);
    menu_event(MENU_SELECT);
    assert(settings_state.shift_threshold == 3250 && !setup_in_workflow());
    assert(settings_writes == 0); /* Commit changes RAM only. */
    menu_event(MENU_SELECT);
    menu_event(MENU_NEXT);
    now += 60000;
    menu_process();
    assert(settings_state.shift_threshold == 3250); /* Idle save discards an unaccepted draft. */
    assert(settings_writes == 1 && preference_writes == 0 && usb_applies == 1);
    assert(menu_parameters_active() && dashboard_state.baccable_dashboard_menu_visible);
}

static void test_permission_guards(void) {
    memset(&settings_state, 0, sizeof(settings_state));
    memset(&chassis_state, 0, sizeof(chassis_state));
    comfort_state.force_q_vexhaust_valve_opened = 0;
    fresh_menu();
    const uint8_t slots[] = {8, 10, 11, 14, 28};
    uint8_t *active[] = {&chassis_state.dyno_mode_enabled_on_master, &chassis_state.front_brake_forced,
                        &chassis_state.awd_sequence, &chassis_state.stability_inverted,
                        &comfort_state.force_q_vexhaust_valve_opened};
    for (unsigned i = 0; i < sizeof(slots); ++i) {
        const SetupParam *param = setup_find_by_flash_index(slots[i]);
        *(uint8_t *)param->value = 1;
        uint8_t page = setup_page_for(slots[i]);
        *active[i] = 1;
        unsigned before = commands;
        setup_select_page(page);
        assert(*(uint8_t *)param->value == 1 && *active[i] == 1 && commands == before);
        setup_render_page(page);
        assert(dashboard_setup_screen[0] == '!');
        *active[i] = 0;
        setup_select_page(page);
        assert(*(uint8_t *)param->value == 0);
    }
}

static void test_steering_menu_ownership(void) {
    fresh_menu();
    settings_state.acc_virtual_pad_enabled = 0;
    settings_state.acc_autostart = 0;
    comfort_state.has_button_press_requested = 0;
    comfort_state.cruise_control_disabled = comfort_state.acc_disabled = 1;
    comfort_state.wheel_pressed_button_id = 0x10;
    telemetry_state.current_rpm_speed = 1000;
    telemetry_state.current_gear = 0;
    bool original = security_state.immobilizer_enabled;
    CAN_RxHeaderTypeDef header = {.DLC = 3};
    uint8_t frame[3] = {0x10, 0, 0};
    vehicle_handle_steering_controls(&header, frame);
    frame[0] = 0x08;
    for (unsigned i = 0; i < 1300; ++i) {
        now += 25;
        vehicle_handle_steering_controls(&header, frame);
    }
    assert(security_state.immobilizer_enabled == original);
    assert(comfort_state.last_pressed_speed_up_wheel_button_duration == 0);
}

static void test_request_cancel_and_failure(void) {
    open_test_action("AWD");
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    assert(chassis_state.awd_sequence == 4);
    now += 1300;
    menu_render();
    assert(strstr(screen, "OFF WAIT"));
    menu_event(MENU_SELECT);
    assert(strstr(screen, "Stop 4WD"));
    menu_event(MENU_SELECT);
    assert(chassis_state.awd_sequence == 0);
    now += 1300;
    menu_render();
    assert(!strstr(screen, "WAIT"));

    open_test_action("Dyno:");
    menu_event(MENU_SELECT);
    menu_event(MENU_SELECT);
    menu_action_reply(C1cmdDynoNotActive); /* Board reports that target was not applied. */
    now += 1300;
    menu_render();
    assert(strstr(screen, "Request failed"));
}

#ifdef MENU_DIAGNOSTICS
static void test_hidden_diagnostics(void) {
    fresh_menu();
    menu_event(MENU_BACK);
    for (unsigned i = 0; i < 4; ++i) menu_event(MENU_NEXT);
    menu_event(MENU_SELECT);
    for (unsigned i = 0; i < 5; ++i) menu_event(MENU_NEXT);
    assert(strstr(screen, "IPC diag"));
    menu_event(MENU_SELECT);
    assert(!strncmp(screen, "20-27 ", 6));
    for (unsigned i = 0; i < 12; ++i) menu_event(MENU_NEXT);
    assert(!strncmp(screen, "80-87 ", 6));
    for (unsigned i = 0; i < 8; ++i) assert((uint8_t)screen[6 + i] == 0x80 + i);
    menu_event(MENU_SELECT);
    assert(strstr(screen, "Display A"));
    menu_event(MENU_BACK);
    assert(strstr(screen, "IPC diag"));
}
#endif

static void test_usb_legacy_and_dependency(void) {
    memset(&settings_state, 0, sizeof(settings_state));
    fresh_menu();
    uint8_t page = setup_page_for(34);
    ibs_override_enable(true);
    setup_select_page(page);
    assert(ibs_override_enabled() && !settings_state.usb_sniffer && !settings_state.usb_elm327);
    ibs_override_enable(false);
    settings_state.usb_sniffer = settings_state.usb_elm327 = 1;
    assert(settings_save() == 0);
    settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    setup_load_from_flash();
    assert(settings_state.usb_sniffer && !settings_state.usb_elm327); /* Existing CAN precedence. */
    uint16_t stored[SETUP_FLASH_PARAM_BUFFER_SIZE] = {0};
    setup_fill_flash_params(stored);
    assert(stored[33] == 1 && stored[34] == 0);
    settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    assert(settings_save() == 0);
}
