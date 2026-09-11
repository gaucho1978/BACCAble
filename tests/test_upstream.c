#include "app/powertrain.h"
#include "diagnostics/fault_reader.h"
#include "features/ibs_override.h"
#include "features/usb_modes.h"
#include "protocol/elm327.h"
#include "transport/diagnostic_link.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>

SettingsState settings_state;
TelemetryState telemetry_state;
RuntimeState runtime_state;
UART_HandleTypeDef huart2;
USBD_HandleTypeDef hUsbDeviceFS;
static bool usb_serial, led_usb;
void led_strip_set_usb(uint8_t enabled) { led_usb = enabled; }
void HAL_GPIO_DeInit(void *port, uint32_t pins) {
    (void)port;
    (void)pins;
}
void usb_device_start(uint8_t serial) {
    usb_serial = serial;
    hUsbDeviceFS.dev_state = 0;
}
void usb_device_stop(void) { hUsbDeviceFS.dev_state = 0; }
void power_wake(void) {}
void board_sync_restart(void) { runtime_state.instruct_slave_boards_trigger_enabled = 1; }
void uart_resume(UART_HandleTypeDef *uart) { (void)uart; }
void parameter_request_cancel(void) {}
void parameter_cache_reset(void) {}
float parameter_cache_get(uint8_t id, uint32_t time) {
    (void)id;
    (void)time;
    return NAN;
}
static uint32_t now, tx_result;
static CAN_TxHeaderTypeDef last_header;
static uint8_t last_data[8];
static unsigned sends, response_mode, rx_count, rx_index, usb_busy;
static CAN_RxHeaderTypeDef replies[4];
static uint8_t reply_data[4][8];
static char output[4096];
static size_t output_length;
static bool diagnostic_mode, diagnostic_busy;
static CAN_HandleTypeDef handle;
uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t delay) { now += delay; }
void status_led_activity(void) {}
void status_led_error(void) {}
void board_uart_process(void) { ++now; }
void board_uart_set_diagnostic(uint8_t enabled) { diagnostic_mode = enabled; }
uint8_t board_uart_diagnostic_send(const uint8_t *data, size_t length) {
    assert(length == UART_BUFFER_SIZE);
    uint8_t checksum = 0;
    for (unsigned i = 0; i < 17; ++i)
        checksum ^= data[i];
    assert(checksum == data[17]);
    return !diagnostic_busy;
}
void can_process(void) { ++now; }
uint32_t can_set_receive_filter(uint32_t id, uint32_t mask, uint8_t ext) {
    (void)id;
    (void)mask;
    (void)ext;
    return HAL_OK;
}
void can_enable(void) {}
void can_disable(void) {}
CAN_HandleTypeDef *can_gethandle(void) { return &handle; }
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *h) {
    (void)h;
    return 3;
}
uint8_t is_can_msg_pending(uint8_t fifo) {
    (void)fifo;
    return rx_index < rx_count;
}
uint32_t can_rx(CAN_RxHeaderTypeDef *h, uint8_t *d) {
    assert(rx_index < rx_count);
    *h = replies[rx_index];
    memcpy(d, reply_data[rx_index++], 8);
    return HAL_OK;
}
uint32_t can_tx(CAN_TxHeaderTypeDef *h, uint8_t *d) {
    last_header = *h;
    memcpy(last_data, d, h->DLC);
    ++sends;
    if (response_mode && h->IDE == CAN_ID_STD && h->StdId == 0x7df) {
        rx_index = 0;
        rx_count = response_mode == 3 ? 3 : 1;
        for (unsigned i = 0; i < 4; ++i)
            replies[i] =
                (CAN_RxHeaderTypeDef){.StdId = 0x7e8, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 8};
        memset(reply_data, 0, sizeof(reply_data));
        if (response_mode == 1) {
            const uint8_t sf[] = {3, 0x41, 0x0c, 0x80};
            memcpy(reply_data[0], sf, sizeof(sf));
        }
        if (response_mode == 2) {
            reply_data[0][0] = 7;
            replies[0].DLC = 4;
        }
        if (response_mode == 3) {
            const uint8_t ff[] = {0x10, 10, 0x62, 0xf1, 0x90, 1, 2, 3};
            const uint8_t other[] = {3, 0x41, 0, 99};
            const uint8_t cf[] = {0x21, 4, 5, 6, 7, 0, 0, 0};
            memcpy(reply_data[0], ff, 8);
            memcpy(reply_data[1], other, sizeof(other));
            memcpy(reply_data[2], cf, 8);
            replies[1].StdId = 0x7e9;
        }
        if (response_mode == 4) {
            reply_data[0][0] = 0x11;
            reply_data[0][1] = 0;
        }
    }
    return tx_result;
}
void cdc_process_tx(void) { ++now; }
void cdc_process(void) { ++now; }
uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t length) {
    if (usb_busy) {
        --usb_busy;
        return USBD_BUSY;
    }
    assert(output_length + length < sizeof(output));
    memcpy(output + output_length, data, length);
    output_length += length;
    output[output_length] = 0;
    return USBD_OK;
}
void _putchar(char c) { (void)c; }

static void fault_reply(const uint8_t *data, unsigned length) {
    CAN_RxHeaderTypeDef h = {.ExtId = 0x18daf140, .IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = length};
    fault_reader_receive(&h, data);
}
static void start_faults(void) {
    now = 100;
    tx_result = HAL_OK;
    response_mode = 0;
    fault_reader_start(0x40);
    fault_reader_process();
    assert(last_header.ExtId == 0x18da40f1 && last_data[1] == 0x10);
    const uint8_t session[] = {2, 0x50, 3};
    fault_reply(session, 3);
    fault_reader_process();
    assert(last_data[1] == 0x19 && last_data[2] == 2 && last_data[3] == 0xff);
}
static void test_faults(void) {
    char text[25];
    start_faults();
    const uint8_t none[] = {3, 0x59, 2, 0xff};
    fault_reply(none, 4);
    assert(!fault_reader_busy() && fault_reader_count() == 0);
    fault_reader_text(0, text, sizeof(text));
    assert(!strcmp(text, "No faults reported"));
    start_faults();
    const uint8_t one[] = {7, 0x59, 2, 0xff, 0x12, 0x34, 0x56, 0x80};
    fault_reply(one, 8);
    assert(fault_reader_count() == 1);
    fault_reader_text(0, text, sizeof(text));
    assert(strstr(text, "P1234-56"));
    start_faults();
    fault_reply(one, 4);
    assert(fault_reader_busy());
    now += 2001;
    fault_reader_process();
    assert(!fault_reader_busy() && !fault_reader_count());
    start_faults();
    const uint8_t first[] = {0x10, 11, 0x59, 2, 0xff, 0x12, 0x34, 0x56};
    fault_reply(first, 8);
    tx_result = HAL_BUSY;
    fault_reader_process();
    assert(fault_reader_busy());
    tx_result = HAL_OK;
    fault_reader_process();
    assert(last_data[0] == 0x30);
    const uint8_t next[] = {0x21, 0x80, 0xc0, 1, 2, 1};
    fault_reply(next, sizeof(next));
    assert(fault_reader_count() == 2);
    fault_reader_text(1, text, sizeof(text));
    assert(strstr(text, "U0001-02"));
    start_faults();
    fault_reply(first, 8);
    fault_reader_process();
    const uint8_t wrong[] = {0x22, 0, 0, 0, 0, 0};
    fault_reply(wrong, sizeof(wrong));
    assert(!fault_reader_busy() && fault_reader_count() == 0);
    start_faults();
    const uint8_t pending[] = {3, 0x7f, 0x19, 0x78};
    for (unsigned i = 0; i < 11; i++) {
        now += 1000;
        fault_reply(pending, 4);
        fault_reader_process();
    }
    assert(!fault_reader_busy());
    /* More than twenty faults are consumed without overflowing the stored result. */
    start_faults();
    uint8_t payload[87] = {0x59, 2, 0xff};
    uint8_t ff[8] = {0x10, sizeof(payload)};
    memcpy(ff + 2, payload, 6);
    fault_reply(ff, 8);
    fault_reader_process();
    unsigned offset = 6, sequence = 1;
    while (offset < sizeof(payload)) {
        uint8_t cf[8] = {0x20 | sequence};
        unsigned n = sizeof(payload) - offset;
        if (n > 7)
            n = 7;
        memcpy(cf + 1, payload + offset, n);
        fault_reply(cf, n + 1);
        offset += n;
        sequence = (sequence + 1) & 15;
    }
    assert(fault_reader_count() == 20);
    fault_reader_text(19, text, sizeof(text));
    assert(strstr(text, "20/20+"));
}
static void command(const char *text) {
    output_length = 0;
    output[0] = 0;
    for (const char *p = text; *p; p++)
        elm327_rx_byte(*p);
    elm327_rx_byte('\r');
    elm327_process();
}
static void test_elm(void) {
    response_mode = 0;
    elmlink_init();
    elm327_set_enabled(1);
    assert(diagnostic_mode);
    command("ATI");
    assert(strstr(output, "ELM327 v1.4") && strchr(output, '>'));
    command("ATE0");
    command("ATH1");
    assert(strstr(output, "OK"));
    usb_busy = 8;
    command("ATI");
    assert(strstr(output, "ELM327 v1.4"));
    command("ATUNKNOWN");
    assert(strchr(output, '?'));
    unsigned previous = sends;
    char oversized[100];
    memset(oversized, '1', 99);
    oversized[99] = 0;
    command(oversized);
    assert(strchr(output, '?') && sends == previous);
    command("010C");
    assert(strstr(output, "NO DATA"));
    response_mode = 1;
    command("010C");
    assert(strstr(output, "41 0C 80"));
    response_mode = 2;
    command("010C");
    assert(strstr(output, "NO DATA"));
    response_mode = 3;
    command("22F190");
    assert(strstr(output, "62 F1 90 01 02 03 04 05 06 07"));
    assert(last_header.StdId == 0x7e0 && last_data[0] == 0x30);
    response_mode = 4;
    command("22F190");
    assert(strstr(output, "NO DATA"));
    response_mode = 0;
    elm327_rx_byte('0');
    elm327_rx_byte('1');
    elm327_rx_lost();
    command("010C");
    assert(strchr(output, '?'));
    command("ATI");
    assert(strstr(output, "ELM327"));
    now += ELM327_IDLE_EXIT_MS + 1;
    elm327_process();
    assert(!elm327_is_enabled() && !diagnostic_mode);
}
static void test_ibs(void) {
    CAN_RxHeaderTypeDef h = {.StdId = 0x41a, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 7};
    uint8_t data[8] = {1, 0x80 | 90, 2, 3, 4, 5, 6, 7};
    telemetry_state.current_rpm_speed = 1000;
    ibs_override_enable(true);
    ibs_override_observe(&h, data);
    now += 3;
    ibs_override_process();
    assert(last_data[1] == (0x80 | 75) && last_header.DLC == 7);
    unsigned previous = sends;
    now += 101;
    ibs_override_process();
    assert(sends == previous);
    h.DLC = 6;
    ibs_override_observe(&h, data);
    ibs_override_process();
    assert(sends == previous);
    telemetry_state.current_rpm_speed = 0;
    ibs_override_process();
    assert(!ibs_override_enabled());
}
static void test_usb_modes(void) {
    output_length = 0;
    settings_state.usb_sniffer = 1;
    settings_state.usb_elm327 = 0;
    usb_modes_apply();
    usb_modes_process();
    assert(usb_modes_active() && usb_serial && led_usb);
    CAN_RxHeaderTypeDef h = {.StdId = 0x123, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 3};
    uint8_t data[8] = {1, 2, 3, 99, 99, 99, 99, 99};
    for (unsigned i = 0; i < 24; i++)
        usb_sniffer_observe(&h, data);
    assert(!output_length);
    hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    usb_modes_process();
    assert(output_length == 64 && (uint8_t)output[0] == 0xa3 && (uint8_t)output[4] == 0x23 && !output[11]);
    usb_sniffer_observe(&h, data);
    for (unsigned i = 0; i < 5; i++) {
        now += 21;
        usb_modes_process();
    }
    assert(output_length == 18 * 16 && (uint8_t)output[16 * 16] == 0xaf && (uint8_t)output[16 * 16 + 4] == 8);
    hUsbDeviceFS.dev_state = 0;
    now += 10001;
    usb_modes_process();
    assert(!usb_modes_active() && !led_usb && !settings_state.usb_sniffer);
    settings_state.usb_elm327 = 1;
    usb_modes_apply();
    usb_modes_process();
    assert(elm327_is_enabled());
    now += 10001;
    usb_modes_process();
    assert(!elm327_is_enabled() && !usb_modes_active());
}
/* Reject a busy configuration send and expose lost remote replies to the client. */
static void test_link_loss(void) {
    elmlink_init();
    elmlink_set_enabled(1);
    diagnostic_busy = true;
    assert(!elmlink_send_config(ELMLINK_BUS_C2, 0x7e8, 0x7ff, 100, 1));
    diagnostic_busy = false;
    assert(elmlink_send_config(ELMLINK_BUS_C2, 0x7e8, 0x7ff, 100, 1));
    uint8_t frame[UART_BUFFER_SIZE] = {0};
    frame[0] = ELMLINK_TO_MASTER;
    frame[1] = ELMLINK_TYPE_RSP;
    frame[5] = 7;
    frame[6] = 0xe8;
    frame[7] = 1;
    frame[8] = 0;
    for (unsigned i = 0; i < 17; ++i)
        frame[17] ^= frame[i];
    for (unsigned i = 0; i < 4; ++i)
        assert(elmlink_on_uart_frame(frame) == 1);
    assert(elmlink_poll(NULL) == 2);
    assert(elmlink_poll(NULL) == 0);
    elmlink_set_enabled(0);
}

int main(void) {
    test_faults();
    test_elm();
    test_usb_modes();
    test_ibs();
    test_link_loss();
    puts("PASS: DTC/ISO-TP bounds, ELM/bridge retries and overflow, USB capture/lifecycle, IBS override");
}
