#include "app/powertrain.h"
#include "features/periodic.h"
#include "features/menu.h"

#if defined(BACCABLE_C1)

/* Restore driver preferences and prepare the main board for vehicle operation. */
void powertrain_init() {
    #ifndef DISABLE_LOW_CONSUME
    power_init();
    #endif
    setup_load_from_flash();
    if (security_state.immobilizer_enabled)
        dashboard_state.execute_dashboard_blinks = 2; // shows the user that the immobilizer is active (or
                                                      // not)

    parameter_page_count = settings_state.is_diesel_enabled ? diesel_page_count : gasoline_page_count;
    // arise trigger to notify enabled functions to slave boards with dedicated messages,after some seconds
    runtime_state.all_processors_wakeup_time = currentTime;
    runtime_state.instruct_slave_boards_trigger_enabled = 1;

    menu_init();
}

/* Run the enabled driving, comfort and display features on the main board. */
void powertrain_process(void) {
    #ifndef DISABLE_LOW_CONSUME
    power_process();
    #endif
    if (telemetry_state.current_rpm_speed == 0)
        chassis_state.stability_inverted = 0;
    exhaust_process();
    board_sync_process();
    led_strip_process();
    drivetrain_process();
    dashboard_feedback_process();
    immobilizer_process();
    start_stop_process();
    seatbelt_process();
    pedal_booster_process();
    dashboard_poll_process();
}

#endif
