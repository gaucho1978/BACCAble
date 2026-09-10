#include "usbd_cdc_if.h"
#include <stdarg.h>
#include <string.h>
#include "protocol/slcan.h"
#if defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    #include "transport/board_uart.h"
#endif

#define USB_TX_SLOTS 8
static usbrx_buf_t received;
static uint8_t rx_packet[RX_BUF_SIZE];
static uint8_t tx_packets[USB_TX_SLOTS][TX_BUF_SIZE];
static uint16_t tx_lengths[USB_TX_SLOTS];
static volatile uint8_t tx_head, tx_tail;
static uint8_t tx_active;
static volatile uint8_t rx_lost;
#if !defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
static uint8_t line[SLCAN_MTU];
#endif
static uint8_t line_length, discard_line;
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Prepare the USB serial connection for a new session. */
static int8_t CDC_Init_FS(void) {
    memset(&received, 0, sizeof(received));
    tx_head = tx_tail = tx_active = rx_lost = line_length = discard_line = 0;
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_packets[0], 0);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_packet);
    return USBD_OK;
}

/* Discard pending replies when the USB serial session ends. */
static int8_t CDC_DeInit_FS(void) {
    tx_head = tx_tail = tx_active = 0;
    return USBD_OK;
}

/* Report the serial settings expected by the USB host. */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *data, uint16_t length) {
    if (cmd == CDC_GET_LINE_CODING && length >= 7) {
        const uint8_t coding[7] = {0x00, 0xc2, 0x01, 0x00, 0, 0, 8};
        memcpy(data, coding, sizeof(coding));
    }
    return USBD_OK;
}

/* Queue incoming USB data for processing and report lost packets. */
static int8_t CDC_Receive_FS(uint8_t *data, uint32_t *length) {
    uint8_t next = (received.head + 1) % NUM_RX_BUFS;
    if (*length > RX_BUF_SIZE || next == received.tail) {
        rx_lost = 1;
        error_assert(ERR_FULLBUF_USBRX);
    } else {
        memcpy(received.buf[received.head], data, *length);
        received.msglen[received.head] = *length;
        __DMB();
        received.head = next;
    }
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, rx_packet);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return USBD_OK;
}

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {CDC_Init_FS, CDC_DeInit_FS, CDC_Control_FS, CDC_Receive_FS};

/* Send queued replies as the USB host becomes ready. */
static void cdc_process_tx(void) {
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    USBD_CDC_HandleTypeDef *cdc = hUsbDeviceFS.pClassData;
    if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED && cdc && !cdc->TxState) {
        if (tx_active) {
            tx_tail = (tx_tail + 1) % USB_TX_SLOTS;
            tx_active = 0;
        }
        if (tx_tail != tx_head) {
            USBD_CDC_SetTxBuffer(&hUsbDeviceFS, tx_packets[tx_tail], tx_lengths[tx_tail]);
            tx_active = USBD_CDC_TransmitPacket(&hUsbDeviceFS) == USBD_OK;
        }
    }
    __set_PRIMASK(irq);
}

/* Process incoming USB commands or pedal-controller data and advance pending replies. */
void cdc_process(void) {
    cdc_process_tx();
    uint8_t packet[RX_BUF_SIZE];
    uint32_t length = 0;
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    if (rx_lost) {
        received.tail = received.head;
        rx_lost = 0;
        line_length = 0;
        discard_line = 1;
    }
    if (received.tail != received.head) {
        length = received.msglen[received.tail];
        memcpy(packet, received.buf[received.tail], length);
        received.tail = (received.tail + 1) % NUM_RX_BUFS;
    }
    __set_PRIMASK(irq);
#if defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    if (length)
        pedal_uart_send(packet, length);
    #ifdef ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER_LOOPBACK
    if (length)
        CDC_Transmit_FS(packet, length);
    #endif
#else
    for (uint32_t i = 0; i < length; ++i) {
        if (packet[i] == '\r') {
    #ifdef ACT_AS_CANABLE
            if (!discard_line && line_length)
                slcan_parse_str(line, line_length);
    #endif
            discard_line = line_length = 0;
        } else if (!discard_line) {
            if (line_length == sizeof(line)) {
                discard_line = 1;
                line_length = 0;
            } else {
                line[line_length++] = packet[i];
            }
        }
    }
#endif
    cdc_process_tx();
}

/* Queue a copy of a USB reply, or report that the connection cannot accept it. */
uint8_t CDC_Transmit_FS(uint8_t *data, uint16_t length) {
    if (!data || !length || length > TX_BUF_SIZE)
        return USBD_FAIL;
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    uint8_t next = (tx_head + 1) % USB_TX_SLOTS;
    uint8_t result = USBD_OK;
    if (!hUsbDeviceFS.pClassData || hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
        result = USBD_FAIL;
    } else if (next == tx_tail) {
        error_assert(ERR_USBTX_BUSY);
        result = USBD_BUSY;
    } else {
        memcpy(tx_packets[tx_head], data, length);
        tx_lengths[tx_head] = length;
        tx_head = next;
    }
    __set_PRIMASK(irq);
    return result;
}

#ifdef DEBUG_MODE
/* Send a bounded diagnostic message to the USB host. */
uint8_t print_to_usb_(char *message) {
    size_t length = strlen(message);
    return CDC_Transmit_FS((uint8_t *)message, length < TX_BUF_SIZE ? length : TX_BUF_SIZE);
}
/* Format and send a bounded diagnostic message to the USB host. */
uint8_t printf_to_usb_(const char *format, ...) {
    char text[TX_BUF_SIZE];
    va_list args;
    va_start(args, format);
    int length = vsnprintf_(text, sizeof(text), format, args);
    va_end(args);
    if (length <= 0)
        return USBD_FAIL;
    size_t used = (size_t)length < sizeof(text) ? (size_t)length : sizeof(text) - 1;
    return CDC_Transmit_FS((uint8_t *)text, (uint16_t)used);
}
#endif
