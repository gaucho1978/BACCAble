#include "features/periodic.h"
/* Runtime capture behavior adapted from gaucho1978 BACCAble, August 2026. */
#include "features/usb_modes.h"
#if !defined(ACT_AS_CANABLE)
    #include "app/powertrain.h"
    #include "protocol/elm327.h"
    #include "transport/diagnostic_link.h"
    #include "usb_device.h"
    #include "usbd_cdc_if.h"
    #include "diagnostics/fault_reader.h"
    #include "features/ibs_override.h"
    #include "diagnostics/parameter_request.h"
    #include "diagnostics/parameter_cache.h"
extern USBD_HandleTypeDef hUsbDeviceFS;
static uint8_t requested, active;
static uint32_t activation, last_flush;

static uint8_t frames[16][16], head, tail, count;
static uint16_t dropped;
    #if defined(BACCABLE_C1)
static uint8_t peer_mask;
static uint32_t peer_seen[2];
    #else
static uint32_t last_presence;
static bool was_connected;
    #endif

/* Report whether a runtime USB function is using this board. */
bool usb_modes_active(void) { return active != 0; }

/* Remember which auxiliary USB connections currently need the boards to stay awake. */
void usb_modes_peer(uint8_t peer, bool connected) {
    #if defined(BACCABLE_C1)
    if (peer > 1)
        return;
    if (connected)
        peer_mask |= 1U << peer;
    else
        peer_mask &= ~(1U << peer);
    peer_seen[peer] = currentTime;
    #else
    (void)peer;
    (void)connected;
    #endif
}

/* Request binary capture on an auxiliary board. */
void usb_modes_set_sniffer(bool enabled) { requested = enabled ? 1 : 0; }

/* Apply saved USB preferences after the user finishes editing settings. */
void usb_modes_apply(void) {
    #if defined(BACCABLE_C1)
    requested = settings_state.usb_sniffer ? 1 : 0;
        #ifdef ACT_AS_ELM327
    if (!requested && settings_state.usb_elm327)
        requested = 2;
        #endif
    #endif
}

/* Retain a capture record until USB accepts its complete contents. */
static void capture_push(const uint8_t *record) {
    memcpy(frames[head], record, 16);
    head = (head + 1) % 16;
    ++count;
}

/* Capture incoming CAN data using upstream's fixed sixteen-byte binary format. */
void usb_sniffer_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
    if (active != 1 || h->DLC > 8 || h->RTR != CAN_RTR_DATA)
        return;
    uint8_t record[16] = {0};
    record[1] = currentTime;
    record[2] = currentTime >> 8;
    record[3] = currentTime >> 16;
    if (dropped && count <= 14) {
        record[0] = 0xaf;
        record[4] = dropped;
        record[5] = dropped >> 8;
        capture_push(record);
        dropped = 0;
    }
    if (count == 16) {
        if (dropped < UINT16_MAX)
            ++dropped;
        return;
    }
    uint32_t id = h->IDE == CAN_ID_EXT ? h->ExtId : h->StdId;
    record[0] = 0xa0 | h->DLC;
    for (unsigned i = 0; i < 4; ++i)
        record[4 + i] = id >> (8 * i);
    memcpy(record + 8, data, h->DLC);
    capture_push(record);
}

/* Change USB roles while releasing the previous session and shared hardware pins. */
static void switch_mode(uint8_t mode) {
    #ifdef ACT_AS_ELM327
    if (active == 2)
        elm327_set_enabled(0);
    #endif
    if (active || mode)
        usb_device_stop();
    #if defined(BACCABLE_C1)
        #if defined(DEBUG_MODE) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER) ||                         \
            defined(ENABLE_USB_MASS_STORAGE)
    led_strip_set_usb(true);
        #else
    led_strip_set_usb(mode != 0);
        #endif
    if (mode) {
        parameter_request_cancel();
        fault_reader_cancel();
        ibs_override_enable(false);
        power_wake();
        runtime_state.low_consume_is_active = 0;
        uart_resume(&huart2);
    }
    #endif
    active = mode;

    activation = last_flush = currentTime;
    head = tail = count = 0;
    dropped = 0;
    if (mode) {
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
        usb_device_start(1);
    #ifdef ACT_AS_ELM327
        if (mode == 2)
            elm327_set_enabled(1);
    #endif
    } else {
    #if defined(ENABLE_USB_MASS_STORAGE) || defined(DEBUG_MODE) ||                                           \
        defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
        MX_USB_DEVICE_Init();
    #endif
    #if defined(BACCABLE_C1)
        parameter_cache_reset();
        board_sync_restart();
        runtime_state.last_received_can_msg_time = currentTime;
    #endif
    }
}

/* Maintain USB sessions and return to normal operation after disconnection or inactivity. */
void usb_modes_process(void) {
    bool connected = hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
    #if defined(BACCABLE_C1)
    for (unsigned i = 0; i < 2; ++i)
        if (currentTime - peer_seen[i] > 4000)
            peer_mask &= ~(1U << i);
    runtime_state.usb_connected_to_slave = peer_mask != 0;
    #else
    if ((connected || was_connected) && (connected != was_connected || currentTime - last_presence >= 1000)) {
        uint8_t presence[] = {C1BusID, C1_CMD_USB_PRESENCE,
        #ifdef BACCABLE_BH
                              1,
        #else
                              0,
        #endif
                              connected};
        if (board_uart_send(presence, sizeof(presence))) {
            runtime_state.we_can_send_a_message_reply = currentTime;
            last_presence = currentTime;
            was_connected = connected;
        }
    }
    runtime_state.usb_connected_to_slave = connected;
    #endif
    if (requested != active)
        switch_mode(requested);
    if (!active)
        return;
    connected = hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
    if (connected) {
        activation = currentTime;
    }
    bool peer_capture = false;
    #if defined(BACCABLE_C1)
    peer_capture = active == 1 && peer_mask;
    #endif
    bool expired = !connected && !peer_capture && (currentTime - activation >= 10000);
    #ifdef ACT_AS_ELM327
    expired |= active == 2 && !elm327_is_enabled();
    #endif
    if (expired) {
        requested = 0;
    #if defined(BACCABLE_C1)
        settings_state.usb_sniffer = settings_state.usb_elm327 = 0;
    #endif
        switch_mode(0);
        return;
    }
    if (!connected) {
    #ifdef ACT_AS_ELM327
        if (active == 2)
            elm327_port_reset();
    #endif
        return;
    }
    cdc_process();
    if (active == 1 && count && (count >= 4 || currentTime - last_flush >= 20)) {
        unsigned n = count < 4 ? count : 4;
        if (n > 16U - tail)
            n = 16U - tail;
        if (CDC_Transmit_FS(frames[tail], n * 16) == USBD_OK) {
            tail = (tail + n) % 16;
            count -= n;
            last_flush = currentTime;
        }
    }
}
#endif
