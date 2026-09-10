#include "app/powertrain.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)

/* Present the currently selected feature setting. */
void dashboard_send_setup(void) {
    setup_render_page(setup_dashboardPageIndex);
    char text[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
    memcpy(text, dashboard_setup_screen, DASHBOARD_MESSAGE_MAX_LENGTH);
    text[DASHBOARD_MESSAGE_MAX_LENGTH] = 0;
    menu_present(text);
}

/* Present the measurements and units belonging to the selected page. */
void dashboard_send_values() {
    char stringToPrint[25];
    dashboard_format_values(
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].name,
        displayed_parameter_values,
        parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index].parameter_ids,
        stringToPrint); // build string to print

    menu_present(stringToPrint);
}

/* Remove BACCAble text from the dashboard. */
void dashboard_clear(void) { menu_present(""); }
#endif
