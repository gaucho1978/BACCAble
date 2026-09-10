#include "app/powertrain.h"
#include "features/periodic.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)

/* Close an expired dashboard session and maintain the active menu. */
void dashboard_poll_process(void) {
    if (dashboard_state.shutdown_dashboard_menu_request_time > 0) {
        if (currentTime - dashboard_state.shutdown_dashboard_menu_request_time > 39000) {
            dashboard_state.baccable_dashboard_menu_visible =
                0;             // stop sending params request when motor is off
            dashboard_clear(); // Clear the displayed text.
            dashboard_state.baccabledashboard_menu_was_visible =
                1; // allows the menu to automatically turn on when motor rotates
            dashboard_state.shutdown_dashboard_menu_request_time = 0; // avoid to return here
        }
    }
    menu_process();
}
#endif
