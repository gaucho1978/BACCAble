#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void drivetrain_process(void) {
    if (settings_state.awd_disabler_enabled == 1) {
        if (chassis_state.awd_sequence > 0) {
            uint8_t tempDeltaTime = 0;
            if (chassis_state.awd_sequence > 1)
                tempDeltaTime = 70;
            if (currentTime - chassis_state.last_sent_drive_train_msg_time >
                (uint32_t)(30 + tempDeltaTime)) { // enter here once each 100msec, then each 30msec
                chassis_state.last_sent_drive_train_msg_time = currentTime;
                status_led_activity();
                can_tx(&chassis_state
                            .drive_train_control_module_reset_msg_header[chassis_state.awd_sequence - 1],
                       chassis_state.drive_train_control_module_reset_msg_data
                           [chassis_state.awd_sequence - 1]); // send drive train control module message
                if (chassis_state.awd_sequence > 1)
                    chassis_state.awd_sequence--;
            }
        }
    }
}
#endif
