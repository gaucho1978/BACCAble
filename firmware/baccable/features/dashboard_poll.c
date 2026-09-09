#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void dashboard_poll_process(void) {
    if (dashboard_state.shutdown_dashboard_menu_request_time > 0) {
        if (currentTime - dashboard_state.shutdown_dashboard_menu_request_time > 39000) {
            dashboard_state.baccable_dashboard_menu_visible =
                0;             // stop sending params request when motor is off
            dashboard_clear(); // ripulisci la stringa
            dashboard_state.baccabledashboard_menu_was_visible =
                1; // allows the menu to automatically turn on when motor rotates
            dashboard_state.shutdown_dashboard_menu_request_time = 0; // avoid to return here
        }
    }
    // send a parameter request each xx msec if dashboard menu shall be visible
    if ((currentTime - diagnostics_state.last_sent_uds_parameter_request_time > 500) &&
        dashboard_state.baccable_dashboard_menu_visible) {
        diagnostics_state.last_sent_uds_parameter_request_time = currentTime;

        switch (dashboard_state.dashboard_menu_indent_level) {
        case 0: // main menu
            dashboard_send_main();
            break;
        case 1:
            if (dashboard_state.main_dashboard_page_index == 1) { // we are in show params submenu
                diagnostics_state.clear_faults_request =
                    0; // ensure we don't perform more tasks simoultaneously
                selected_parameter_element = !selected_parameter_element;
                if (parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index]
                        .parameter_ids[selected_parameter_element] ==
                    parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index]
                        .parameter_ids[!selected_parameter_element]) {
                    selected_parameter_element = 0; // single param
                }

                if (parameter_definitions[parameter_pages[settings_state.is_diesel_enabled]
                                                         [dashboard_state.dashboard_page_index]
                                                             .parameter_ids[selected_parameter_element]]
                        .request_id > 0xFF) { // if req id is greather than 0xFF it is a standard UDS request.
                    parameter_request_begin();
                } else { //<0xff request_id means special value that we use to get particular values
                    displayed_parameter_values[selected_parameter_element] = native_parameter_read(
                        (uint8_t)parameter_pages[settings_state.is_diesel_enabled][dashboard_state
                                                                                       .dashboard_page_index]
                            .parameter_ids[selected_parameter_element]); // aquire param in a variabile
                    dashboard_send_values();                             // Send params to BH board
                }
            }

            if (dashboard_state.main_dashboard_page_index == 9) {
                dashboard_send_setup();
            }
            if (dashboard_state.main_dashboard_page_index == 10) {
                dashboard_send_parameter_setup();
            }
            break;
        default:
            break; // unexpected
        }
    }
}
#endif
