#include "app/powertrain.h"
#include "features/parking.h"
#include "features/parking_mirrors.h"
#include "state/parking.h"
#include "transport/diagnostic_link.h"
#include <assert.h>
#include <stdio.h>
SettingsState settings_state;
TelemetryState telemetry_state;
MirrorsState mirrors_state;
RuntimeState runtime_state;
UART_HandleTypeDef huart2;
static uint32_t now, result;
static unsigned sends, link_sends;
static uint8_t last_data[8], last_link[UART_BUFFER_SIZE];
static CAN_TxHeaderTypeDef last_header;
static bool can_pending;
uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t delay) { now += delay; }
void can_process(void) { ++now; }
uint32_t can_set_receive_filter(uint32_t id, uint32_t mask, uint8_t ext) {
    (void)id;
    (void)mask;
    (void)ext;
    return HAL_OK;
}
void can_enable(void) {}
void status_led_activity(void) {}
void board_uart_set_diagnostic(uint8_t enabled) { (void)enabled; }
uint8_t board_uart_diagnostic_send(const uint8_t *data, size_t size) {
    assert(size == UART_BUFFER_SIZE);
    memcpy(last_link, data, size);
    ++link_sends;
    return 1;
}
uint32_t can_tx(CAN_TxHeaderTypeDef *h, uint8_t *d) {
    last_header = *h;
    memcpy(last_data, d, h->DLC);
    if (result == HAL_OK)
        ++sends;
    return result;
}
uint8_t is_can_msg_pending(uint8_t fifo) {
    (void)fifo;
    return can_pending;
}
uint32_t can_rx(CAN_RxHeaderTypeDef *h, uint8_t *d) {
    assert(can_pending);
    can_pending = false;
    *h = (CAN_RxHeaderTypeDef){.StdId = 0x7e8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 8};
    memset(d, 0, 8);
    d[0] = 3;
    d[1] = 0x41;
    return HAL_OK;
}
static void observe(uint32_t id, uint8_t *data, unsigned length) {
    CAN_RxHeaderTypeDef h = {.StdId = id, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = length};
    parking_observe(&h, data);
}
#if defined(BACCABLE_C2)
static void test_parking(void) {
    memset(&parking_state, 0, sizeof(parking_state));
    now = 100;
    result = HAL_OK;
    uint8_t data[8] = {0};
    parking_set_options(true, false);
    observe(0x54a, data, 4);
    parking_process();
    assert(sends == 0);
    observe(0xfc, data, 4);
    data[4] = 0x20;
    observe(0x1f5, data, 5);
    data[0] = 0x40;
    observe(0x3e7, data, 6);
    result = HAL_BUSY;
    parking_process();
    assert(!parking_state.pdc_owned);
    result = HAL_OK;
    parking_process();
    assert(last_header.StdId == 0x5b0 && last_data[1] == 0x20 && parking_state.pdc_owned);
    now += 50;
    parking_set_options(false, false);
    parking_process();
    assert(!last_data[1] && !parking_state.pulse);
    memset(data, 0, 8);
    data[3] = 0x40;
    observe(0x54a, data, 4);
    parking_process();
    assert(last_data[1] == 0x20 && !parking_state.pdc_owned);
    now += 50;
    parking_process();
    unsigned previous = sends;
    parking_set_options(true, false);
    parking_process();
    assert(sends == previous);
    /* An old gear/brake report cannot trigger a fresh button press. */
    now += 4000;
    data[3] = 0;
    observe(0x54a, data, 4);
    parking_process();
    assert(sends == previous);
}
#else
static void gear(uint8_t gear_value) {
    uint8_t data[8] = {0};
    data[3] = gear_value;
    telemetry_state.current_gear = gear_value;
    observe(0x3e8, data, 8);
    data[3] = 0;
    data[4] = 125;
    data[5] = 0;
    observe(0x3e6, data, 6);
}
static void audio(bool muted) {
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 0, 0};
    observe(0x358, data, 6);
    data[0] = muted ? 0x30 : 0x10;
    observe(0x5be, data, 1);
}
static void test_parking(void) {
    memset(&parking_state, 0, sizeof(parking_state));
    now = 100;
    result = HAL_OK;
    gear(0x0e);
    audio(false);
    parking_set_options(false, true);
    parking_process();
    assert(last_header.StdId == 0x358 && last_data[2] == 0xe0);
    now += 50;
    parking_set_options(false, false);
    parking_process();
    assert(last_data[2] == 3);
    audio(true);
    gear(0x0d);
    parking_process();
    assert(last_data[2] == 0xe0 && parking_state.audio_owned == 0);
    now += 50;
    parking_process();
    /* A driver who unmutes during reverse retains that choice through the maneuver. */
    audio(false);
    gear(0x0e);
    parking_set_options(false, true);
    parking_process();
    now += 50;
    parking_process();
    audio(true);
    audio(false);
    unsigned previous = sends;
    parking_process();
    assert(sends == previous);
    gear(2);
    parking_process();
    now += 3000;
    gear(2);
    audio(false);
    parking_process();
    assert(sends == previous);
    /* Mirrors wait between commands, cancel delayed returns on reverse re-entry and restore after drive. */
    settings_state.park_mirror = 1;
    mirrors_state.turn_indicator = 1;
    mirrors_state.park_mirror_msg_header =
        (CAN_TxHeaderTypeDef){.StdId = 0x123, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4};
    mirrors_state.right_park_mirror_horizontal_pos = 10;
    mirrors_state.right_mirror_horizontal_operative_pos = 30;
    gear(0x0e);
    parking_mirrors_process();
    assert(mirrors_state.store_operative_mirror_position);
    mirrors_state.store_operative_mirror_position = 0;
    previous = sends;
    now += 2400;
    gear(0x0e);
    parking_mirrors_process();
    assert(sends == previous);
    now += 101;
    gear(0x0e);
    parking_mirrors_process();
    assert(last_data[2] == 10);
    gear(2);
    parking_mirrors_process();
    now += 9000;
    gear(0x0e);
    parking_mirrors_process();
    assert(!mirrors_state.restore_operative_mirrors_position);
    gear(2);
    parking_mirrors_process();
    now += 10001;
    gear(2);
    parking_mirrors_process();
    assert(mirrors_state.restore_operative_mirrors_position);
    now += 2500;
    gear(2);
    parking_mirrors_process();
    assert(last_data[2] == 30);
}
#endif
static void frame(uint8_t *f, unsigned type, unsigned flags, uint32_t id, unsigned length) {
    memset(f, 0, UART_BUFFER_SIZE);
    f[0] = ELMLINK_SLAVE_ID;
    f[1] = type;
    f[2] = flags;
    f[3] = id >> 24;
    f[4] = id >> 16;
    f[5] = id >> 8;
    f[6] = id;
    f[7] = length;
    f[16] = 42;
    for (unsigned i = 0; i < 17; i++)
        f[17] ^= f[i];
}
static void test_link(void) {
    uint8_t f[UART_BUFFER_SIZE];
    now = 100;
    result = HAL_OK;
    elmlink_init();
    frame(f, ELMLINK_TYPE_ARM, ELMLINK_FLAG_ARM_ON, 0, 0);
    assert(elmlink_on_uart_frame(f) == 1 && elmlink_is_enabled());
    frame(f, ELMLINK_TYPE_REQ, 0, 0x7e0, 9);
    assert(elmlink_on_uart_frame(f) == 2);
    frame(f, ELMLINK_TYPE_REQ, 0, 0x7e0, 2);
    f[17] ^= 1;
    assert(elmlink_on_uart_frame(f) == 2);
    frame(f, ELMLINK_TYPE_REQ, 0, 0x800, 2);
    assert(elmlink_on_uart_frame(f) == 2);
    frame(f, ELMLINK_TYPE_REQ, 0, 0x7e0, 2);
    assert(elmlink_on_uart_frame(f) == 1);
    elmlink_process();
    assert(last_header.StdId == 0x7e0 && last_header.DLC == 2);
    can_pending = true;
    elmlink_process();
    assert(last_link[1] == ELMLINK_TYPE_RSP && last_link[16] == 42);
    now += 50;
    elmlink_process();
    assert(last_link[1] == ELMLINK_TYPE_END && !(last_link[2] & ELMLINK_FLAG_NODATA));
    frame(f, ELMLINK_TYPE_REQ, 0, 0x7e0, 2);
    assert(elmlink_on_uart_frame(f) == 1);
    elmlink_process();
    now += 400;
    elmlink_process();
    assert(last_link[1] == ELMLINK_TYPE_END && (last_link[2] & ELMLINK_FLAG_NODATA));
    now += 150001;
    elmlink_process();
    assert(!elmlink_is_enabled());
}
int main(void) {
    test_parking();
    test_link();
    puts("PASS: parking button ownership/release, freshness, diagnostic bridge validation/response/timeout");
}
