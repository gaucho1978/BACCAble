#include "app/main.h"
void board_commands_dispatch(const uint8_t *message) {
    switch (message[0]) {
    case C1BusID: // message directed to baccable connected to C1 bus

#if defined(BACCABLE_C1)
        switch (message[1]) {
        case C1cmdForceFrontBrake:
            chassis_state.front_brake_forced = 1;    // update status
            chassis_state.launch_assist_enabled = 1; // enable launch assist
            break;
        case C1cmdNormalFrontBrake:
            chassis_state.front_brake_forced = 0;
            chassis_state.launch_assist_enabled = 0; // disable launch assist
            break;
        case C1cmdDynoActive:
            chassis_state.dyno_mode_enabled_on_master = 1; // dyno active
            break;
        case C1cmdDynoNotActive:
            chassis_state.dyno_mode_enabled_on_master = 0; // dyno not active
            break;
        case C1cmdLaneDoubleTap:
            if (settings_state.has_function_enabled)
                comfort_state.has_button_press_requested = 5; // press HAS for 5 times (5 messages)
            break;
        case C1cmdLaneSingleTap: // request to C2 to execute the ESC/TC toggle
            // status_led_error();
            uint8_t tmpArr2[2] = {C2BusID, C2cmdtoggleEscTc};
            board_uart_send(tmpArr2, 2);
            break;
        case C1usbConnected: // usb connected to slave. avoid sleep
            runtime_state.usb_connected_to_slave = 1;
            break;
        default:
            break;
        }
        status_led_activity();
#endif
        break;
    case C2BusID: // message directed to baccable connected to C2 bus
#if defined(BACCABLE_C2)
        switch (message[1]) {
        case C2cmdtoggleDyno: // dyno request
            if (chassis_state.front_brake_forced == 0)
                chassis_toggle_dyno();
            break;
        case C2cmdtoggleEscTc: // ESC/TC request
            // if we can, enable it
            if (settings_state.esc_tc_customizator_enabled) {
                if (!(chassis_state.dyno_mode_enabled || chassis_state.dyno_state_machine != 0xff))
                    chassis_state.stability_inverted = !chassis_state.stability_inverted;
                uint8_t tmpArr1[2] = {C1_Bh_BusID, C1BHcmdShowRaceScreen};
                if (chassis_state.stability_inverted &&
                    settings_state.show_race_mask) { // if enabled and race screen requested, notify C1 and BH
                    board_uart_send(tmpArr1, 2);
                } else {
                    tmpArr1[1] = C1BHcmdStopShowRaceScreen;
                    board_uart_send(tmpArr1, 2);
                }
            }
            break;
        case C2cmdForceFrontBrake: // force front brake
            chassis_state.front_brake_forced = 5;
            break;
        case C2cmdNormalFrontBrake: // release the brake - we set it to 255, just to trigger serial msg
                                    // sending in the main (sync) and not here (async), since async may cause
                                    // concurrent variables write
            chassis_state.front_brake_forced = 255;
            break;
        case C2cmdRaceMaskDefault: // do not request to show race mask
            settings_state.show_race_mask = 0;
            break;
        case C2cmdShowRaceMask: // request to show race mask
            settings_state.show_race_mask = 1;
            break;
        case C2cmdToggleHas: // request to press HAS button for more consecutive messages
            comfort_state.has_button_press_requested = 5;
            break;
        case C2cmdGetStatus:
            // nothing to do, since we just need to set weCanSendAMessageReply
            break;
        default:
            break;
        }

        status_led_activity();
        runtime_state.we_can_send_a_message_reply = currentTime;
#endif

        break;
    case BhBusID: // message directed to baccable connected to BH bus
#if defined(BACCABLE_BH)
        switch (message[1]) {
        case BHcmdOdometerBlinkDisable: // odometer blink disable request
            display_state.disable_odometer_blink = 1;
            break;
        case BHcmdOdometerBlinkDefault: // odometer blink default request
            display_state.disable_odometer_blink = 0;
            break;
        case BHcmdFunctParkMirrorDisabled: // park mirror disable
            settings_state.park_mirror = 0;
            mirrors_state.park_mirror_operative_position_not_stored = 1;
            break;
        case BHcmdFunctParkMirrorEnabled: // park mirror enabled
            settings_state.park_mirror = 1;
            break;
        case BHcmdFunctParkMirrorStoreCurPos: // park mirror enabled and transfer current mirror position
            settings_state.park_mirror = 1;
            mirrors_state.store_current_park_mirror_position = 1;
            break;
        default:
        }
        runtime_state.we_can_send_a_message_reply = currentTime;
        status_led_activity();
#endif
        break;

    case BhBusIDparamString: // message directed to baccable connected to BH bus in order to transfer a
                             // parameter to print
#if defined(BACCABLE_BH)
        memcpy(&dashboard_state.dashboard_page_string_array[0], &message[1],
               DASHBOARD_MESSAGE_MAX_LENGTH); // copy array that we will use in the main

        if (chassis_state.stability_inverted) { // if esc/tc is active, don't show baccable menu
            display_state.request_to_send_one_frame = 0;
        } else {
            if (display_state.request_to_send_one_frame <= 2)
                display_state.request_to_send_one_frame += 1; // Send one frame
        }
        runtime_state.we_can_send_a_message_reply = currentTime;
        status_led_activity();
#endif
        break;

    case BhBusIDgetStatus:
#if defined(BACCABLE_BH)
        runtime_state.we_can_send_a_message_reply = currentTime;
        status_led_activity();
#endif
        break;

    case BhBusChimeRequest:
#if defined(BACCABLE_BH)
        runtime_state.we_can_send_a_message_reply = currentTime;
        display_state.request_to_play_chime = 1;
        status_led_activity();
#endif
        break;

    case AllSleep: // message directed to all the modules, in order to request low consumption
        // Not used for now..
        break;
    case C2BusIDAllSleepAck: // message directed to baccable on C1 bus, generated by C2 baccable, to inform
                             // that he received the sleep request
        // if we are the baccable on C1 bus
        break;
    case BHBusIDAllSleepAck: // message directed to baccable on C1 bus, generated by BH baccable, to inform
                             // that he received the sleep request
        // if we are the baccable on C1 bus
        // Not used for now..
        break;
    case AllResetFaults: // message received by baccable on BH and C2 bus. we shall reset all faults
#if (defined(BACCABLE_C2) || defined(BACCABLE_BH))
        diagnostics_state.clear_faults_request = 255;

    #if defined(BACCABLE_C2)
        runtime_state.we_can_send_a_message_reply = currentTime; // we decided that only C2 is allowed to
                                                                 // reply
    #endif

        status_led_activity();
#endif
        break;

    case C1_C2_BusID: // message received by C1 and C2 baccable.
#if (defined(BACCABLE_C1) || defined(BACCABLE_C2))
        if (message[1] == C1_C2_cmdLaneDoubleTap) {
            if (settings_state.has_function_enabled)
                comfort_state.has_button_press_requested = 5; // press HAS for 5 times (5 messages)
        }
        status_led_activity();
#endif
        break;

    case C1_Bh_BusID: // message received by C1 and BH baccable.
#if (defined(BACCABLE_C1) || defined(BACCABLE_BH))
                      // #if defined(BACCABLE_C1)
        if (message[1] == C1BHcmdShowRaceScreen) {
            if (settings_state.esc_tc_customizator_enabled)
                chassis_state.stability_inverted = 1;
        }
        if (message[1] == C1BHcmdStopShowRaceScreen) {
            chassis_state.stability_inverted = 0;
        }
        status_led_activity();
#endif
        break;

    case C2_Bh_BusID: // message directed to baccable connected to C2 and BH bus
#if defined(BACCABLE_C2) || defined(BACCABLE_BH)
        switch (message[1]) {
        case C2_Bh_cmdSetPedalBoostStatus:
            // third byte contains the pedal booster status
            settings_state.pedal_booster_enabled = message[2];
            break;
        case C2_Bh_cmdFunctHAS_Disabled:
            settings_state.has_function_enabled = 0;
            break;
        case C2_Bh_cmdFunctHAS_Enabled:
            settings_state.has_function_enabled = 1;
            break;
        case C2_Bh_cmdFunction_ESC_TC_Disabled:
            settings_state.esc_tc_customizator_enabled = 0;
            chassis_state.stability_inverted = 0;
            break;
        case C2_Bh_cmdFunction_ESC_TC_Enabled:
            settings_state.esc_tc_customizator_enabled = 1;
            break;
        case C2_Bh_cmdFunction_Save_Log_to_File:
            filesystem_save_log();
            break;
        default:
        }

        status_led_activity();
    #if defined(BACCABLE_C2)
        runtime_state.we_can_send_a_message_reply =
            currentTime; // we decided that only C2 can reply, otherwise errors may arise
    #endif
#endif
        break;

    default:
        // not expected to end up here
        break;
    }
}
