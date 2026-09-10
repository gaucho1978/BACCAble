#include "platform/power.h"

#if defined(BACCABLE_C1)

/* Prepare control of auxiliary-board power and low-consumption operation. */
void power_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = CHIP_LOW_CONSUME_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // open drain so that when we set it to 1 it stays float (the
                                                // other chip will set it to 3,3V)
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    power_release_slaves();

    GPIO_InitStruct.Pin = CAN_LOW_CONSUME_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // we need to be able to set it to 3,3V
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    power_transceivers_wake();
}

/* Keep auxiliary boards stopped while the main board prepares them. */
void power_hold_slaves_in_reset(void) {
    HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 0); // resets the other chips
}

/* Allow auxiliary boards to start operating. */
void power_release_slaves(void) {
    HAL_GPIO_WritePin(CHIP_LOW_CONSUME, 1); // remove reset of other chips
}

/* Place the vehicle-bus interfaces in their low-consumption state. */
void power_transceivers_sleep(void) {
    HAL_GPIO_WritePin(CAN_LOW_CONSUME, 1); // set other can transceivers to Sleep
}

/* Restore the vehicle-bus interfaces for normal operation. */
void power_transceivers_wake(void) {
    HAL_GPIO_WritePin(CAN_LOW_CONSUME, 0); // set other can transceivers to wakeUp
}

/* Choose sleep or wake operation from recent vehicle activity. */
void power_process(void) {

    if (runtime_state.low_consume_is_active) { // se siamo in basso consumo

        // Wake the other boards when recent vehicle traffic is present.
        if (currentTime - runtime_state.last_received_can_msg_time <
            TIMING__C1____CAN_ACTIVITY_WINDOW_FOR_WAKEUP_MS) {

            power_wake();

            uart_resume(&huart2); // restart serial line between chips

            runtime_state.low_consume_is_active = 0;

            runtime_state.all_processors_wakeup_time = currentTime;
            runtime_state.instruct_slave_boards_trigger_enabled = 1;
        }
    } else { // Otherwise enter low-consumption mode if needed.
        // Enter low-consumption mode after the inactivity timeout.
        if (currentTime - runtime_state.last_received_can_msg_time >
            TIMING__C1____CAN_INACTIVITY_TIMEOUT_BEFORE_SLEEP_MS) {
            if (runtime_state.usb_connected_to_slave == 0) {
                runtime_state.low_consume_is_active = 1;

                uart_pause(&huart2); // stop serial line between chips

                power_sleep();
            }
        }
    }
}

/* Suspend enabled features and auxiliary boards during vehicle inactivity. */
void power_sleep(void) {

    power_hold_slaves_in_reset();         // reduce consumption of other chips (left under reset)
    chassis_state.front_brake_forced = 0; // ensure we disabled relative functions status in master baccable
    chassis_state.dyno_mode_enabled_on_master = 0; // ensure we disabled dyno status on master baccable too
    status_led_activity();
}

/* Restore enabled features and auxiliary boards when vehicle activity returns. */
void power_wake(void) {
    if (runtime_state.low_consume_is_active) {
        power_transceivers_wake(); // wake up transceivers
        power_release_slaves();    // wake up other processors
        status_led_activity();
    }
}
#endif
