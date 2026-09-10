#include "transport/can_bus.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

CAN_TypeDef fake_can;
uint32_t fake_primask;
static uint32_t start_result, tx_result, init_result, filter_result, free_mailboxes = 3;
static unsigned sent_count, requested_fifo;
static CAN_TxHeaderTypeDef sent_header;
static uint8_t sent_data[8];
static bool single_mailbox;
void HAL_GPIO_Init(void *port, GPIO_InitTypeDef *config) {
    (void)port;
    (void)config;
}
void HAL_NVIC_SetPriority(int irq, int preempt, int sub) {
    (void)irq;
    (void)preempt;
    (void)sub;
}
void HAL_NVIC_EnableIRQ(int irq) { (void)irq; }
uint32_t HAL_CAN_Init(CAN_HandleTypeDef *can) {
    (void)can;
    return init_result;
}
uint32_t HAL_CAN_ConfigFilter(CAN_HandleTypeDef *can, CAN_FilterTypeDef *filter) {
    (void)can;
    (void)filter;
    return filter_result;
}
uint32_t HAL_CAN_Start(CAN_HandleTypeDef *can) {
    (void)can;
    return start_result;
}
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *can) {
    (void)can;
    return free_mailboxes;
}
uint32_t HAL_CAN_AddTxMessage(CAN_HandleTypeDef *can, CAN_TxHeaderTypeDef *header, uint8_t *data,
                              uint32_t *mailbox) {
    (void)can;
    (void)mailbox;
    if (tx_result != HAL_OK)
        return tx_result;
    sent_header = *header;
    memcpy(sent_data, data, header->DLC);
    ++sent_count;
    if (single_mailbox)
        --free_mailboxes;
    return HAL_OK;
}
uint32_t HAL_CAN_GetRxMessage(CAN_HandleTypeDef *can, uint32_t fifo, CAN_RxHeaderTypeDef *header,
                              uint8_t *data) {
    (void)can;
    (void)fifo;
    (void)data;
    header->DLC = 9;
    return HAL_OK;
}
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *can, uint32_t fifo) {
    (void)can;
    requested_fifo = fifo;
    return 1;
}
void status_led_activity(void) {}
void status_led_error(void) {}
void error_assert(error_t error) { (void)error; }

static void test_can(void) {
    CAN_TxHeaderTypeDef header = {.StdId = 0x123, .DLC = 8};
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    can_init();
    assert(can_tx(&header, data) == HAL_ERROR);
    can_set_silent(1);
    can_set_bitrate(CAN_BITRATE_1000K);
    can_set_autoretransmit(0);
    can_enable();
    assert(can_gethandle()->Init.Mode == CAN_MODE_SILENT);
    assert(can_gethandle()->Init.Prescaler == 6);
    assert(!can_gethandle()->Init.AutoRetransmission);
    can_set_bitrate(CAN_BITRATE_10K);
    assert(can_gethandle()->Init.Prescaler == 6);
    assert(can_tx(&header, data) == HAL_OK);
    data[0] = 99;
    tx_result = HAL_BUSY;
    can_process();
    assert(sent_count == 0);
    tx_result = HAL_OK;
    can_process();
    assert(sent_count == 1 && sent_data[0] == 1);
    can_process();
    assert(sent_count == 1);
    header.DLC = 9;
    assert(can_tx(&header, data) == HAL_ERROR);
    header.DLC = 8;
    header.StdId = 0x800;
    assert(can_tx(&header, data) == HAL_ERROR);
    header.StdId = 0x123;
    header.TransmitGlobalTime = 2;
    assert(can_tx(&header, data) == HAL_ERROR);
    header.TransmitGlobalTime = DISABLE;
    for (unsigned i = 0; i < TXQUEUE_LEN - 1; ++i)
        assert(can_tx(&header, data) == HAL_OK);
    assert(can_tx(&header, data) == HAL_ERROR);
    free_mailboxes = 0;
    can_process();
    assert(sent_count == 1);
    can_disable();
    free_mailboxes = 3;
    can_enable();
    can_process();
    assert(sent_count == 1);
    CAN_RxHeaderTypeDef received = {
        .ExtId = 0x18daf110, .IDE = CAN_ID_EXT, .RTR = CAN_RTR_REMOTE, .DLC = 8, .Timestamp = 123};
    assert(can_forward(&received, data) == HAL_OK);
    can_process();
    assert(sent_header.ExtId == received.ExtId && sent_header.IDE == CAN_ID_EXT);
    assert(sent_header.RTR == CAN_RTR_REMOTE && !sent_header.TransmitGlobalTime);
    assert(is_can_msg_pending(1) && requested_fifo == 1);
    assert(can_rx(&received, data) == HAL_ERROR);
    assert(can_rx(NULL, data) == HAL_ERROR);
    can_disable();
    uint32_t *failures[] = {&init_result, &filter_result, &start_result};
    for (unsigned i = 0; i < 3; ++i) {
        *failures[i] = HAL_ERROR;
        can_enable();
        assert(can_tx(&header, data) == HAL_ERROR);
        *failures[i] = HAL_OK;
    }
}

static void test_can_queue_roundtrip(void) {
    can_enable();
    single_mailbox = true;
    /* Several full passes cross the ring boundary with different frame types and lengths. */
    for (unsigned pass = 0; pass < 3; ++pass) {
        unsigned before = sent_count;
        for (unsigned i = 0; i < TXQUEUE_LEN - 1; ++i) {
            CAN_TxHeaderTypeDef header = {.StdId = 0x7ff - i,
                                          .ExtId = 0x1fffffff - i,
                                          .IDE = i % 2 ? CAN_ID_EXT : CAN_ID_STD,
                                          .RTR = i & 2 ? CAN_RTR_REMOTE : CAN_RTR_DATA,
                                          .DLC = i % 9,
                                          .TransmitGlobalTime = i & 4 ? ENABLE : DISABLE};
            uint8_t data[8];
            for (unsigned j = 0; j < sizeof(data); ++j)
                data[j] = i ^ j ^ pass;
            assert(can_tx(&header, data) == HAL_OK);
            memset(data, 0xff, sizeof(data)); /* The queue owns a copy. */
        }
        free_mailboxes = 1;
        tx_result = pass % 2 ? HAL_ERROR : HAL_BUSY;
        can_process();
        assert(sent_count == before);
        tx_result = HAL_OK;
        for (unsigned i = 0; i < TXQUEUE_LEN - 1; ++i) {
            free_mailboxes = 1;
            can_process();
            assert(sent_count == before + i + 1);
            assert(sent_header.DLC == i % 9);
            assert(sent_header.IDE == (i % 2 ? CAN_ID_EXT : CAN_ID_STD));
            assert((i % 2 ? sent_header.ExtId : sent_header.StdId) == (i % 2 ? 0x1fffffff : 0x7ff) - i);
            assert(sent_header.RTR == (i & 2 ? CAN_RTR_REMOTE : CAN_RTR_DATA));
            assert(sent_header.TransmitGlobalTime == (i & 4 ? ENABLE : DISABLE));
            for (unsigned j = 0; j < sent_header.DLC; ++j)
                assert(sent_data[j] == (i ^ j ^ pass));
        }
    }
    can_disable();
    single_mailbox = false;
}

USBD_HandleTypeDef hUsbDeviceFS;
static USBD_CDC_HandleTypeDef usb;
static uint8_t *active_usb_data;
static uint16_t active_usb_length;
static unsigned parsed_lines, usb_transmits;
static char parsed[31];
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *device, uint8_t *data, uint16_t length) {
    (void)device;
    active_usb_data = data;
    active_usb_length = length;
    return USBD_OK;
}
uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *device, uint8_t *data) {
    (void)device;
    (void)data;
    return USBD_OK;
}
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *device) {
    (void)device;
    return USBD_OK;
}
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *device) {
    (void)device;
    usb.TxState = 1;
    ++usb_transmits;
    return USBD_OK;
}
int8_t slcan_parse_str(uint8_t *data, uint8_t length) {
    assert(!fake_primask);
    assert(length < sizeof(parsed));
    memcpy(parsed, data, length);
    parsed[length] = 0;
    ++parsed_lines;
    return 0;
}
static void receive(const char *data) {
    uint32_t length = strlen(data);
    USBD_Interface_fops_FS.Receive((uint8_t *)data, &length);
}
static void test_usb(void) {
    uint8_t data[] = "abc";
    assert(CDC_Transmit_FS(data, 3) == USBD_FAIL);
    cdc_process();
    hUsbDeviceFS.pClassData = &usb;
    hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    USBD_Interface_fops_FS.Init();
    assert(CDC_Transmit_FS(data, 3) == USBD_OK);
    data[0] = 'z';
    cdc_process();
    assert(active_usb_length == 3 && !memcmp(active_usb_data, "abc", 3));
    for (unsigned i = 0; i < 6; ++i)
        assert(CDC_Transmit_FS(data, 3) == USBD_OK);
    assert(CDC_Transmit_FS(data, 3) == USBD_BUSY);
    cdc_process();
    assert(usb_transmits == 1 && !memcmp(active_usb_data, "abc", 3));
    usb.TxState = 0;
    cdc_process();
    assert(usb_transmits == 2 && !memcmp(active_usb_data, "zbc", 3));
    fake_primask = 1;
    assert(CDC_Transmit_FS(data, 3) == USBD_OK);
    assert(fake_primask == 1);
    fake_primask = 0;
    receive("t123");
    cdc_process();
    assert(!parsed_lines);
    receive("1AA\rO\r");
    cdc_process();
    assert(parsed_lines == 2 && !strcmp(parsed, "O"));
    receive("123456789012345678901234567890123456\rC\r");
    cdc_process();
    assert(parsed_lines == 3 && !strcmp(parsed, "C"));
    for (unsigned i = 0; i < NUM_RX_BUFS + 1; ++i)
        receive("t1231");
    cdc_process();
    receive("AA\rO\r");
    cdc_process();
    assert(parsed_lines == 4 && !strcmp(parsed, "O"));
    USBD_Interface_fops_FS.DeInit();
    hUsbDeviceFS.pClassData = NULL;
    cdc_process();
    assert(CDC_Transmit_FS(data, 3) == USBD_FAIL);
}
int main(void) {
    test_can();
    test_can_queue_roundtrip();
    test_usb();
    puts("PASS: CAN validation/retry/silent mode, USB TX ownership/RX overflow/IRQ state");
    return 0;
}
