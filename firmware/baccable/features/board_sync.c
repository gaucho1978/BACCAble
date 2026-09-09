#include "app/powertrain.h"
#include "features/periodic.h"
#include "diagnostics/parameter_request.h"
#if defined(BACCABLE_C1)
void board_sync_process(void) {
    if (runtime_state.instruct_slave_boards_trigger_enabled) {
        if ((currentTime - runtime_state.all_processors_wakeup_time) >
            TIMING__C1____DELAY_BEFORE_SERIAL_INSTRUCT_OF_C2BH_AFTER_OTHER_CHIP_WAKE_MS) {
            // status_led_blink_error(5);
            runtime_state.instruct_slave_boards_trigger_enabled = 0; // avoid to return here

            // send messages to slave boards
            uint8_t tmpArr0[2] = {C2_Bh_BusID, C2_Bh_cmdFunction_ESC_TC_Enabled};
            if (!settings_state.esc_tc_customizator_enabled)
                tmpArr0[1] = C2_Bh_cmdFunction_ESC_TC_Disabled;
            board_uart_send(tmpArr0, 2);

            // send messages to slave boards
            uint8_t tmpArr1[3] = {C2_Bh_BusID, C2_Bh_cmdSetPedalBoostStatus,
                                  settings_state.pedal_booster_enabled};
            board_uart_send(tmpArr1, 3);

            // send the message to BH to inform about the status of the function disable_odometer_blink
            uint8_t tmpArr2[2] = {BhBusID, BHcmdOdometerBlinkDefault};
            if (settings_state.disable_odometer_blink)
                tmpArr2[1] = BHcmdOdometerBlinkDisable;
            board_uart_send(tmpArr2, 2);

            // Now let's inform the C2 Baccable
            uint8_t tmpArr3[2] = {C2BusID, C2cmdRaceMaskDefault};
            if (settings_state.show_race_mask)
                tmpArr3[1] = C2cmdShowRaceMask;
            board_uart_send(tmpArr3, 2);

            // Now let's inform the BH Baccable
            uint8_t tmpArr4[2] = {BhBusID, BHcmdFunctParkMirrorDisabled};
            if (settings_state.park_mirror)
                tmpArr4[1] = BHcmdFunctParkMirrorEnabled;
            board_uart_send(tmpArr4, 2);

            // notify to C2 and BH HAS function status
            uint8_t tmpArr5[2] = {C2_Bh_BusID, C2_Bh_cmdFunctHAS_Disabled};
            if (settings_state.has_function_enabled)
                tmpArr5[1] = C2_Bh_cmdFunctHAS_Enabled;
            board_uart_send(tmpArr5, 2);
        }
    }
}
#endif
