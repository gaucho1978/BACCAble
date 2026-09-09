#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void led_strip_process(void) {
    if (settings_state.led_strip_controller_enabled == 1) {
        // don't act as canable. One USB port pin is used to control leds.
        led_strip_init(); // initialize leds strip controller - this is called many times to divide the
                          // operations on more loops
        if (display_state.leds_strip_is_on) { // if the strip is on,
            if (currentTime - comfort_state.time_since_last_received_accelerator_message >
                10000) { // if no can interesting message for 10 seconds,  shutdown the leds to save energy
                led_strip_shutdown();
                display_state.leds_strip_is_on = 0; // entriamo solo una volta
            }
        }
    }
}
#endif
