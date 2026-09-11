#include "app/main.h"
#include "stm32f0xx_it.h"
#include "diagnostics/parameter_cache.h"
#include "features/parking.h"
#include "features/ibs_override.h"
#include "features/usb_modes.h"
#include "protocol/elm327.h"
#include "transport/diagnostic_link.h"

/* Prepare this board's enabled features and restore its saved preferences. */
static void application_init(void) {
    SystemClock_Config();
    status_led_init();
    can_init();
#if defined(BACCABLE_C1)
    powertrain_init();
#endif
    uart_init();
#if defined(BACCABLE_BH) || defined(BACCABLE_C2)
    filesystem_init();
#endif
#if defined(ACT_AS_CANABLE) || defined(DEBUG_MODE) || defined(ENABLE_USB_MASS_STORAGE) ||                    \
    defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    if (RCC->CSR & RCC_CSR_PORRSTF) {
    #if defined(BACCABLE_C1)
        led_strip_set_usb(true);
    #endif
        MX_USB_DEVICE_Init();
    }
    RCC->CSR |= RCC_CSR_RMVF;
#endif
#if defined(BACCABLE_C1) || defined(BACCABLE_C2)
    can_set_bitrate(CAN_BITRATE_500K);
    can_enable();
#endif
#if defined(BACCABLE_BH)
    body_init();
#endif
#if !defined(ACT_AS_CANABLE)
    elmlink_init();
    usb_modes_apply();
#endif
}

/* Deliver incoming vehicle updates while leaving time for other device features. */
static void receive_can_frames(void) {
    /* A bounded batch drains the hardware FIFO without starving periodic work. */
    for (unsigned budget = 0; budget < 8 && is_can_msg_pending(CAN_RX_FIFO0); ++budget) {
        CAN_RxHeaderTypeDef header = {0};
        uint8_t data[8] = {0};
        if (can_rx(&header, data) != HAL_OK)
            continue;
#if defined(BACCABLE_C1)
        runtime_state.last_received_can_msg_time = currentTime;
#endif
#if defined(ACT_AS_CANABLE)
        uint8_t text[SLCAN_MTU];
        int8_t length = slcan_parse_frame(text, &header, data);
        if (length > 0)
            CDC_Transmit_FS(text, (uint16_t)length);
#else
        usb_sniffer_observe(&header, data);
        parking_observe(&header, data);
    #if defined(BACCABLE_C1)
        ibs_override_observe(&header, data);
        if (header.IDE == CAN_ID_STD && header.RTR == CAN_RTR_DATA && header.StdId == 0x41a &&
            header.DLC >= 2) {
            parameter_cache_put(95, data[0], currentTime);
            parameter_cache_put(96, data[1], currentTime);
        }
    #endif
        if (header.RTR != CAN_RTR_DATA || header.DLC == 0)
            continue;
        if (header.IDE == CAN_ID_EXT)
            vehicle_dispatch_diagnostic(&header, data);
        if (header.IDE == CAN_ID_STD)
            vehicle_dispatch_standard(&header, data);
#endif
#if defined(BACCABLE_C1)
        if (header.IDE == CAN_ID_STD)
            parameter_cache_observe(header.StdId, header.DLC);
#endif
        can_process();
    }
}

/* Continue the requested fault-clearing sequence across vehicle controllers. */
static void clear_faults_process(void) {
#if defined(BACCABLE_C1) || defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (!diagnostics_state.clear_faults_request ||
        currentTime - diagnostics_state.last_sent_clear_faults_msg <= 25)
        return;
    #if defined(BACCABLE_C1)
    if (diagnostics_state.clear_faults_request == 255) {
        uint8_t command[] = {AllResetFaults};
        board_uart_send(command, sizeof(command));
    }
    #endif
    diagnostics_state.clear_faults_msg_header.ExtId =
        0x18DA00F1 | ((uint32_t)diagnostics_state.clear_faults_request << 8);
    if (can_tx(&diagnostics_state.clear_faults_msg_header, diagnostics_state.clear_faults_msg_data) ==
        HAL_OK) {
        --diagnostics_state.clear_faults_request;
        diagnostics_state.last_sent_clear_faults_msg = currentTime;
    }
#endif
}

/* Keep vehicle communication, user controls and enabled features running. */
int main(void) {
    application_init();
    for (;;) {
        uint32_t started = currentTime;
        board_uart_process();
#if !defined(ACT_AS_CANABLE)
        usb_modes_process();
    #ifdef ACT_AS_ELM327
        if (elm327_is_enabled()) {
            elm327_process();
            can_process();
            status_led_process();
            continue;
        }
    #endif
        elmlink_process();
        if (elmlink_is_enabled()) {
            can_process();
            status_led_process();
            continue;
        }
#endif
        receive_can_frames();
#if defined(BACCABLE_C1)
        pedal_uart_process();
        powertrain_process();
        ibs_override_process();
#elif defined(BACCABLE_C2)
        chassis_process();
        parking_process();
#elif defined(BACCABLE_BH)
        body_process();
        parking_process();
#endif

#if defined(ACT_AS_CANABLE) || defined(DEBUG_MODE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
        cdc_process();
#endif
        clear_faults_process();
        can_process();
        status_led_process();
        if (currentTime - started > 2)
            status_led_error();
    }
}
