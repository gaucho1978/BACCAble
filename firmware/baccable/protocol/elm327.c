/* Adapted from gaucho1978 BACCAble 02b2fd8; see LICENSE.MD and UPSTREAM_SYNC.md. */

#include "protocol/elm327.h"
#ifdef ACT_AS_ELM327
    #include <string.h>
    #include <math.h>
    #include "diagnostics/parameter_cache.h"
    #include "third_party/printf/printf.h"
    #include "app/application_state.h"
    #include "usbd_cdc_if.h"
    #include "platform/status_led.h"
    #if defined(BACCABLE_C1)
        #include "transport/diagnostic_link.h"
        #include "transport/board_uart.h"
    #endif
    #ifdef ELM327_STRICT_ELM_DEFAULTS
        #define ELM_DEF_ECHO 1
        #define ELM_DEF_LF 0
        #ifndef ELM327_EXTRA_CR_BEFORE_PROMPT
            #define ELM327_EXTRA_CR_BEFORE_PROMPT
        #endif
    #else
        #define ELM_DEF_ECHO 0
        #define ELM_DEF_LF 1
    #endif
static uint8_t echoOn = ELM_DEF_ECHO;
static uint8_t headersOn = 0;
static uint8_t linefeedOn = ELM_DEF_LF;
static uint8_t spacesOn = 1;
static uint8_t cafOn = 1;
static uint8_t cfcOn = 1;
static uint8_t adaptTiming = 1;
static uint32_t canFilterValue = 0;
static uint32_t canFilterMask = 0;
static uint32_t canSendHeader = 0x7DF;
static uint8_t extendedHeader = 0;
static uint8_t canPriority = 0x18;
static uint16_t cmdTimeout = ELM327_DEFAULT_TIMEOUT_MS;
static uint16_t rspTimeout = ELM327_DEFAULT_TIMEOUT_MS;
static uint8_t pp2C = 0x00;
static uint8_t pp2D = 0x01;
static uint8_t pp2E = 0x00;
static uint8_t pp2F = 0x01;
static uint8_t variableDlc = 0;
static uint8_t busOpen = 0;
static uint8_t busDivisor = 0;
    #if defined(BACCABLE_C1)
static uint8_t elmModeOn = 0;
static uint32_t elmLastCmd = 0;
    #endif
    #if defined(BACCABLE_C1)
static uint8_t activeBus = ELMLINK_BUS_LOCAL;
static uint8_t remoteFailed;
static uint8_t remoteEnded = 1;
typedef struct {
    uint16_t addr;
    uint8_t bus;
} elm_route_t;
static elm_route_t routeCache[ELM327_ROUTE_CACHE_LEN];
static uint8_t routeCount = 0;
typedef struct {
    uint32_t id;
    uint8_t ext;
    uint8_t dlc;
    uint8_t data[8];
} elm_remote_frame_t;
static elm_remote_frame_t remoteFifo[ELM327_REMOTE_FIFO_LEN];
static uint8_t remoteHead = 0, remoteTail = 0;
/* Keep an auxiliary-board reply until the diagnostic client can consume it. */
static void elm_remote_push(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc) {
    if (dlc > 8)
        return;
    uint8_t next = (uint8_t)((remoteHead + 1) % ELM327_REMOTE_FIFO_LEN);
    if (next == remoteTail) {
        remoteFailed = 1;
        remoteEnded = 1;
        return;
    }
    remoteFifo[remoteHead].id = id;
    remoteFifo[remoteHead].ext = ext;
    remoteFifo[remoteHead].dlc = (dlc > 8) ? 8 : dlc;
    memset(remoteFifo[remoteHead].data, 0, 8);
    memcpy(remoteFifo[remoteHead].data, d, dlc);
    remoteHead = next;
}
/* Discard replies left over from the previous diagnostic request. */
static void elm_remote_clear(void) {
    remoteHead = remoteTail = 0;
    remoteFailed = 0;
}
/* Identify the controller selected by the diagnostic client. */
static uint16_t elm_target_addr(void) {
    if (extendedHeader)
        return (uint16_t)((canSendHeader >> 8) & 0xFF);
    return (uint16_t)(canSendHeader & 0x7FF);
}
/* Find the bus on which this controller last answered. */
static uint8_t elm_route_lookup(uint16_t addr, uint8_t *bus) {
    for (uint8_t i = 0; i < routeCount; i++) {
        if (routeCache[i].addr == addr) {
            *bus = routeCache[i].bus;
            return 1;
        }
    }
    return 0;
}
/* Remember a responding controller to avoid repeating bus discovery. */
static void elm_route_store(uint16_t addr, uint8_t bus) {
    for (uint8_t i = 0; i < routeCount; i++) {
        if (routeCache[i].addr == addr) {
            routeCache[i].bus = bus;
            return;
        }
    }
    if (routeCount < ELM327_ROUTE_CACHE_LEN) {
        routeCache[routeCount].addr = addr;
        routeCache[routeCount].bus = bus;
        routeCount++;
    }
}
/* Prefer the known controller location or search the available vehicle buses. */
static uint8_t elm_bus_order(uint8_t *order) {
    uint8_t known;
    if (elm_route_lookup(elm_target_addr(), &known)) {
        order[0] = known;
        return 1;
    }
    if (busDivisor > 1) {
        order[0] = ELMLINK_BUS_BH;
        order[1] = ELMLINK_BUS_C2;
        order[2] = ELMLINK_BUS_LOCAL;
    } else {
        order[0] = ELMLINK_BUS_LOCAL;
        order[1] = ELMLINK_BUS_C2;
        order[2] = ELMLINK_BUS_BH;
    }
    return 3;
}
    #endif
static uint8_t protoNum = 6;
static uint8_t protoAuto = 1;
static uint32_t lastRxByteTime = 0;
static uint32_t fcHeader = 0;
static uint8_t fcHeaderExt = 0;
static uint8_t fcData[8] = {0x30, 0x00, 0x00, 0, 0, 0, 0, 0};
static uint8_t fcDataLen = 3;
static uint8_t fcMode = 0;
static volatile uint8_t rxRing[ELM327_RX_RING_LEN];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;
static char cmdBuf[ELM327_CMD_BUF_LEN];
static uint8_t cmdLen = 0;
static uint8_t discardCommand, outputFailed;
static uint8_t txChunk[ELM327_TX_CHUNK_LEN];
static uint8_t txChunkLen = 0;
    #ifndef ELM327_TRACE_DISABLE
static char traceBuf[ELM327_TRACE_LEN];
static uint16_t traceHead = 0;
static uint8_t traceWrapped = 0;
static uint8_t traceDumping = 0;
/* Retain diagnostic activity for an optional troubleshooting dump. */
static void trace_char(char c) {
    if (traceDumping)
        return;
    traceBuf[traceHead++] = c;
    if (traceHead >= ELM327_TRACE_LEN) {
        traceHead = 0;
        traceWrapped = 1;
    }
}
/* Retain a diagnostic message for an optional troubleshooting dump. */
static void trace_str(const char *s) {
    while (*s)
        trace_char(*s++);
}
    #else
        #define trace_char(c)                                                                                \
            do {                                                                                             \
            } while (0)
        #define trace_str(s)                                                                                 \
            do {                                                                                             \
            } while (0)
    #endif
/* Deliver a complete output chunk, allowing queued USB transfers to finish. */
static void elm_flush(void) {
    if (!txChunkLen)
        return;
    uint32_t started = currentTime;
    uint8_t result;
    do {
        cdc_process_tx();
        result = CDC_Transmit_FS(txChunk, txChunkLen);
    } while (result == USBD_BUSY && currentTime - started < 500);
    if (result != USBD_OK)
        outputFailed = 1;
    txChunkLen = 0;
}
/* Append a character to the current diagnostic response. */
static void elm_putc(char c) {
    if (outputFailed)
        return;
    trace_char(c);
    txChunk[txChunkLen++] = (uint8_t)c;
    if (txChunkLen >= ELM327_TX_CHUNK_LEN)
        elm_flush();
}
/* Append readable text to the current diagnostic response. */
static void elm_puts(const char *s) {
    while (*s)
        elm_putc(*s++);
}
/* Present a response byte as two hexadecimal characters. */
static void elm_puthex(uint8_t v) {
    const char *h = "0123456789ABCDEF";
    elm_putc(h[(v >> 4) & 0x0F]);
    elm_putc(h[v & 0x0F]);
}
/* End a response line using the client-selected line endings. */
static void elm_eol(void) {
    elm_putc('\r');
    if (linefeedOn)
        elm_putc('\n');
}
/* Send one complete response line to the diagnostic client. */
static void elm_line(const char *s) {
    elm_puts(s);
    elm_eol();
}
/* Recognize a hexadecimal character in a client command. */
static uint8_t elm_hexval(char c) {
    if (c >= '0' && c <= '9')
        return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F')
        return (uint8_t)(c - 'A' + 10);
    return 0xFF;
}
/* Reject diagnostic requests containing non-hexadecimal characters. */
static uint8_t elm_is_hex_string(const char *s) {
    if (strlen(s) < 2)
        return 0;
    while (*s) {
        if (elm_hexval(*s) == 0xFF)
            return 0;
        s++;
    }
    return 1;
}
/* Read a hexadecimal command value after its format has been checked. */
static uint32_t elm_parse_hex(const char *s) {
    uint32_t v = 0;
    while (*s) {
        uint8_t n = elm_hexval(*s);
        if (n == 0xFF)
            break;
        v = (v << 4) | n;
        s++;
    }
    return v;
}
static void elm_bus_open(void);
/* Send a diagnostic message to a controller on this board. */
static uint8_t elm_can_send_frame_id(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext) {
    CAN_TxHeaderTypeDef h;
    uint8_t data[8];
    elm_bus_open();
    if (len > 8)
        len = 8;
    memset(data, 0, sizeof(data));
    memcpy(data, bytes, len);
    h.RTR = CAN_RTR_DATA;
    h.DLC = len;
    h.TransmitGlobalTime = DISABLE;
    if (ext) {
        h.IDE = CAN_ID_EXT;
        h.ExtId = id & 0x1FFFFFFF;
        h.StdId = 0;
    } else {
        h.IDE = CAN_ID_STD;
        h.StdId = id & 0x7FF;
        h.ExtId = 0;
    }
    if (can_tx(&h, data) != HAL_OK)
        return 0;
    uint32_t t0 = currentTime;
    while (currentTime - t0 < 20) {
        can_process();
        if (HAL_CAN_GetTxMailboxesFreeLevel(can_gethandle()) == 3)
            return 1;
    }
    return 1;
}
/* Send a diagnostic message through the selected vehicle bus. */
static uint8_t elm_bus_send(const uint8_t *bytes, uint8_t len, uint32_t id, uint8_t ext) {
    #if defined(BACCABLE_C1)
    if (activeBus != ELMLINK_BUS_LOCAL) {
        remoteEnded = 0;
        return elmlink_send_request(activeBus, id, ext, bytes, len);
    }
    #endif
    return elm_can_send_frame_id(bytes, len, id, ext);
}
/* Keep diagnostic communication moving while waiting for a controller. */
static void elm_bus_pump(void) {
    board_uart_process();
    cdc_process_tx();
    #if defined(BACCABLE_C1)
    if (activeBus != ELMLINK_BUS_LOCAL) {
        uint8_t result = elmlink_poll(elm_remote_push);
        if (result)
            remoteEnded = 1;
        if (result == 2)
            remoteFailed = 1;
        return;
    }
    #endif
    can_process();
}
/* Allow extra reply time when another board carries the request. */
static uint16_t elm_wait_budget(void) {
    #if defined(BACCABLE_C1)
    if (activeBus != ELMLINK_BUS_LOCAL)
        return (uint16_t)(rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS);
    #endif
    return rspTimeout;
}
/* Report that an auxiliary board has finished collecting replies. */
static uint8_t elm_remote_finished(void) {
    #if defined(BACCABLE_C1)
    return (activeBus != ELMLINK_BUS_LOCAL && remoteEnded) ? 1 : 0;
    #endif
    return 0;
}
    #if defined(BACCABLE_C1)
/* Finish the previous auxiliary-board exchange before accepting another command. */
static void elm_remote_drain(void) {
    if (activeBus == ELMLINK_BUS_LOCAL || remoteEnded)
        return;
    uint32_t t0 = currentTime;
    while (currentTime - t0 < (uint32_t)rspTimeout + ELMLINK_DEFAULT_TIMEOUT_MS) {
        uint8_t result = elmlink_poll(elm_remote_push);
        if (result) {
            remoteEnded = 1;
            remoteFailed |= result == 2;
            break;
        }
    }
    remoteEnded = 1;
}
    #else
        #define elm_remote_drain()                                                                           \
            do {                                                                                             \
            } while (0)
    #endif
/* Send the next part of a request to the selected controller. */
static uint8_t elm_can_send_frame(const uint8_t *data8) {
    return elm_bus_send(data8, 8, canSendHeader, extendedHeader);
}
/* Make the local vehicle connection available to diagnostics. */
static void elm_bus_open(void) {
    if (busOpen)
        return;
    #if !defined(BACCABLE_C1)
    can_enable();
    #endif
    busOpen = 1;
}
/* Release the diagnostic connection when the session ends. */
static void elm_bus_close(void) {
    if (!busOpen)
        return;
    #if !defined(BACCABLE_C1)
    can_disable();
    #endif
    busOpen = 0;
}
/* Use the requested protocol speed to prioritize bus discovery. */
static void elm_apply_bitrate_divisor(uint8_t divisor) {
    if (divisor == 0)
        divisor = 1;
    busDivisor = divisor;
    #if defined(BACCABLE_C1)
    return;
    #else
    uint8_t wasOpen = busOpen;
    elm_bus_close();
    can_set_prescaler((uint32_t)12 * divisor);
    if (wasOpen)
        elm_bus_open();
    #endif
}
/* Accept only replies matching the diagnostic client's controller filter. */
static uint8_t elm_filter_pass(const CAN_RxHeaderTypeDef *h) {
    if (canFilterMask == 0)
        return 1;
    uint32_t id = (h->IDE == CAN_ID_EXT) ? h->ExtId : h->StdId;
    return ((id & canFilterMask) == (canFilterValue & canFilterMask)) ? 1 : 0;
}
/* Retrieve the next eligible controller reply from the selected bus. */
static uint8_t elm_can_get_frame(CAN_RxHeaderTypeDef *h, uint8_t *d) {
    #if defined(BACCABLE_C1)
    if (activeBus != ELMLINK_BUS_LOCAL) {
        while (remoteTail != remoteHead) {
            elm_remote_frame_t *f = &remoteFifo[remoteTail];
            remoteTail = (uint8_t)((remoteTail + 1) % ELM327_REMOTE_FIFO_LEN);
            memset(h, 0, sizeof(*h));
            h->RTR = CAN_RTR_DATA;
            h->DLC = f->dlc;
            if (f->ext) {
                h->IDE = CAN_ID_EXT;
                h->ExtId = f->id;
            } else {
                h->IDE = CAN_ID_STD;
                h->StdId = f->id;
            }
            memcpy(d, f->data, 8);
            if (!elm_filter_pass(h))
                continue;
            return 1;
        }
        return 0;
    }
    #endif
    for (unsigned budget = 0; budget < 16 && is_can_msg_pending(CAN_RX_FIFO0); ++budget) {
        if (can_rx(h, d) != HAL_OK)
            continue;
        if (h->RTR != CAN_RTR_DATA || h->DLC == 0 || h->DLC > 8)
            continue;
        if (!elm_filter_pass(h))
            continue;
        return 1;
    }
    return 0;
}
/* Show the responding controller address when the client requests headers. */
static void elm_print_header(const CAN_RxHeaderTypeDef *h) {
    if (!headersOn)
        return;
    #if defined(ELM327_HEADER_ALWAYS_SPACED)
    const uint8_t sep = 1;
    #else
    const uint8_t sep = spacesOn;
    #endif
    if (h->IDE == CAN_ID_EXT) {
        elm_puthex((h->ExtId >> 24) & 0xFF);
        if (sep)
            elm_putc(' ');
        elm_puthex((h->ExtId >> 16) & 0xFF);
        if (sep)
            elm_putc(' ');
        elm_puthex((h->ExtId >> 8) & 0xFF);
        if (sep)
            elm_putc(' ');
        elm_puthex(h->ExtId & 0xFF);
        if (sep)
            elm_putc(' ');
    } else {
    #ifdef ELM327_HEADER_3DIGITS
        const char *hx = "0123456789ABCDEF";
        elm_putc(hx[(h->StdId >> 8) & 0x0F]);
        elm_putc(hx[(h->StdId >> 4) & 0x0F]);
        elm_putc(hx[h->StdId & 0x0F]);
        if (sep)
            elm_putc(' ');
    #else
        elm_puthex((h->StdId >> 8) & 0xFF);
        if (sep)
            elm_putc(' ');
        elm_puthex(h->StdId & 0xFF);
        if (sep)
            elm_putc(' ');
    #endif
    }
}
/* Ask the responding controller to continue a multipart reply. */
static void elm_send_flow_control(const CAN_RxHeaderTypeDef *response) {
    #if defined(BACCABLE_C1)
    if (activeBus != ELMLINK_BUS_LOCAL)
        return;
    #endif
    uint8_t fc[8];
    uint8_t len = 8;
    memset(fc, ELM327_PAD_BYTE, sizeof(fc));
    if (fcMode && fcDataLen) {
        memcpy(fc, fcData, fcDataLen);
    } else {
        fc[0] = 0x30;
        fc[1] = 0x00;
        fc[2] = 0x00;
    }
    if (fcMode && fcHeader)
        elm_bus_send(fc, len, fcHeader, fcHeaderExt);
    else {
        uint32_t id = response->IDE == CAN_ID_EXT
                          ? (response->ExtId & 0x1fff0000U) | ((response->ExtId & 0xff) << 8) |
                                ((response->ExtId >> 8) & 0xff)
                      : response->StdId >= 8 ? response->StdId - 8
                                             : canSendHeader;
        elm_bus_send(fc, len, id, response->IDE == CAN_ID_EXT);
    }
}
/* Restore the diagnostic interpreter's initial command preferences. */
static void elm_reset_defaults(void) {
    echoOn = ELM_DEF_ECHO;
    headersOn = 0;
    linefeedOn = ELM_DEF_LF;
    spacesOn = 1;
    cafOn = 1;
    cfcOn = 1;
    adaptTiming = 1;
    canSendHeader = 0x7DF;
    extendedHeader = 0;
    canFilterValue = 0;
    canFilterMask = 0;
    cmdTimeout = ELM327_DEFAULT_TIMEOUT_MS;
    canPriority = 0x18;
    fcHeader = 0;
    fcHeaderExt = 0;
    fcData[0] = 0x30;
    fcData[1] = 0x00;
    fcData[2] = 0x00;
    fcDataLen = 3;
    fcMode = 0;
    protoNum = 6;
    protoAuto = 1;
    variableDlc = 0;
    elm_bus_close();
    #if defined(BACCABLE_C1)
    activeBus = ELMLINK_BUS_LOCAL;
    rspTimeout = cmdTimeout;
    elm_remote_clear();
    #endif
    cmdLen = 0;
}
/* Forget incomplete commands and replies after a USB session changes. */
void elm327_port_reset(void) {
    discardCommand = outputFailed = 0;
    #if defined(BACCABLE_C1)
    if (!elmModeOn)
        return;
    #endif
    trace_str("\r\n== serial session reset ==");
    elm_bus_close();
    rxHead = rxTail;
    cmdLen = 0;
    txChunkLen = 0;
}
/* Prepare a fresh ELM-compatible diagnostic interpreter. */
void elm327_init(void) {
    elm_reset_defaults();
    discardCommand = outputFailed = 0;
    rxHead = 0;
    rxTail = 0;
    txChunkLen = 0;
}
    #if defined(BACCABLE_C1)
/* Enter or leave exclusive vehicle diagnostics across the three boards. */
void elm327_set_enabled(uint8_t on) {
    on = on ? 1 : 0;
    if (on == elmModeOn)
        return;
    elmModeOn = on;
    elmLastCmd = currentTime;
    elm327_init();
    rxHead = rxTail = 0;
        /* USB lifecycle is owned by the runtime USB mode manager. */
        #if defined(BACCABLE_C1)
    elmlink_set_enabled(on);
    if (on) {
        elmlink_send_arm(1);
    } else {
        elmlink_send_arm(0);
        elmlink_set_enabled(0);
    }
        #endif
}
/* Report whether an ELM diagnostic session currently owns vehicle communication. */
uint8_t elm327_is_enabled(void) { return elmModeOn; }
    #endif
/* Collect client input while rejecting commands damaged by receive overflow. */
void elm327_rx_byte(uint8_t c) {
    #if defined(BACCABLE_C1)
    if (!elmModeOn)
        return;
    #endif
    uint16_t next = (uint16_t)((rxHead + 1) % ELM327_RX_RING_LEN);
    if (next == rxTail) {
        elm327_rx_lost();
        return;
    }
    rxRing[rxHead] = c;
    rxHead = next;
}
/* Discard a damaged command through its next line boundary. */
void elm327_rx_lost(void) {
    rxTail = rxHead;
    cmdLen = 0;
    discardCommand = 1;
}

/* Take the next waiting character from the diagnostic client. */
static int16_t elm_ring_get(void) {
    if (rxTail == rxHead)
        return -1;
    uint8_t c = rxRing[rxTail];
    rxTail = (uint16_t)((rxTail + 1) % ELM327_RX_RING_LEN);
    return (int16_t)c;
}
/* Apply a supported adapter command or report that it is unsupported. */
static void elm_handle_at(const char *at) {
    if (!strcmp(at, "Z")) {
        elm_reset_defaults();
        HAL_Delay(50);
        elm_puts(ELM327_ID_STRING);
        elm_eol();
        return;
    }
    if (!strcmp(at, "WS")) {
        HAL_Delay(50);
        elm_line(ELM327_ID_STRING);
        return;
    }
    if (!strcmp(at, "I")) {
        elm_line(ELM327_ID_STRING);
        return;
    }
    if (!strcmp(at, "@1")) {
        elm_line(ELM327_DESCR_STRING);
        return;
    }
    if (!strcmp(at, "@2")) {
        elm_line("?");
        return;
    }
    if (!strcmp(at, "D")) {
        elm_reset_defaults();
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "E0")) {
        echoOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "E1")) {
        echoOn = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "L0")) {
        linefeedOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "L1")) {
        linefeedOn = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "H0")) {
        headersOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "H1")) {
        headersOn = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "S0")) {
        spacesOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "S1")) {
        spacesOn = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "V0")) {
        variableDlc = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "V1")) {
        variableDlc = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "AR") || !strcmp(at, "AL") || !strcmp(at, "NL") || !strcmp(at, "BI") ||
        !strcmp(at, "PC") || !strcmp(at, "MA")) {
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "DP")) {
        switch (protoNum) {
        case 6:
            elm_line("ISO 15765-4 (CAN 11/500)");
            break;
        case 7:
            elm_line("ISO 15765-4 (CAN 29/500)");
            break;
        case 8:
            elm_line("ISO 15765-4 (CAN 11/250)");
            break;
        case 9:
            elm_line("ISO 15765-4 (CAN 29/250)");
            break;
        case 0xA:
            elm_line("SAE J1939 (CAN 29/250)");
            break;
        case 0xB:
        case 0xC: {
            uint8_t div = (protoNum == 0xB) ? pp2D : pp2F;
            uint16_t kb = (uint16_t)(500 / (div ? div : 1));
            elm_puts((protoNum == 0xB) ? "USER1 (CAN " : "USER2 (CAN ");
            if (kb >= 100)
                elm_putc((char)('0' + (kb / 100)));
            if (kb >= 10)
                elm_putc((char)('0' + ((kb / 10) % 10)));
            elm_putc((char)('0' + (kb % 10)));
            elm_line(")");
            break;
        }
        default:
            elm_line("ISO 15765-4 (CAN 29/" ELM327_BITRATE_STR ")");
            break;
        }
        return;
    }
    if (!strcmp(at, "DPN")) {
        const char *hx = "0123456789ABCDEF";
        if (protoAuto)
            elm_putc('A');
        elm_putc(hx[protoNum & 0x0F]);
        elm_eol();
        return;
    }
    if (!strcmp(at, "RV")) {
        float voltage = parameter_cache_get(35, currentTime);
        if (isfinite(voltage)) {
            char text[16];
            snprintf_(text, sizeof(text), "%.1fV", (double)voltage);
            elm_line(text);
        } else
            elm_line("NO DATA");
        return;
    }
    if (!strcmp(at, "CAF0")) {
        cafOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "CAF1")) {
        cafOn = 1;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "CFC0")) {
        cfcOn = 0;
        elm_line("OK");
        return;
    }
    if (!strcmp(at, "CFC1")) {
        cfcOn = 1;
        elm_line("OK");
        return;
    }
    if (strlen(at) == 3 && at[0] == 'A' && at[1] == 'T') {
        uint8_t v = (uint8_t)(at[2] - '0');
        if (v <= 2) {
            adaptTiming = v;
            elm_line("OK");
        } else {
            elm_line("?");
        }
        return;
    }
    if (!strncmp(at, "ST", 2)) {
        if (strlen(at + 2) != 2 || !elm_is_hex_string(at + 2)) {
            elm_line("?");
            return;
        }
        uint32_t v = elm_parse_hex(at + 2);
        cmdTimeout = (v == 0) ? ELM327_DEFAULT_TIMEOUT_MS : (uint16_t)(v * 4);
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "CP", 2)) {
        if (strlen(at + 2) != 2 || !elm_is_hex_string(at + 2) || elm_parse_hex(at + 2) > 0x1f) {
            elm_line("?");
            return;
        }
        canPriority = (uint8_t)elm_parse_hex(at + 2);
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "CRA", 3)) {
        const char *a = at + 3;
        uint8_t n = (uint8_t)strlen(a);
        if (n == 0) {
            canFilterValue = 0;
            canFilterMask = 0;
        } else {
            uint32_t val = 0, msk = 0;
            for (uint8_t i = 0; i < n; i++) {
                uint8_t d = elm_hexval(a[i]);
                val <<= 4;
                msk <<= 4;
                if (d != 0xFF) {
                    val |= d;
                    msk |= 0x0F;
                }
            }
            canFilterValue = val;
            canFilterMask = msk;
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "SH", 2)) {
        const char *a = at + 2;
        uint8_t n = (uint8_t)strlen(a);
        if ((n != 3 && n != 6 && n != 8) || !elm_is_hex_string(a) ||
            elm_parse_hex(a) > (n == 3   ? 0x7ffU
                                : n == 8 ? 0x1fffffffU
                                         : 0xffffffU)) {
            elm_line("?");
            return;
        }
        uint32_t addr = elm_parse_hex(a);
        if (n > 3) {
            canSendHeader = (n == 8) ? addr : (((uint32_t)canPriority << 24) | (addr & 0x00FFFFFF));
            extendedHeader = 1;
        } else {
            canSendHeader = addr;
            extendedHeader = 0;
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "FCSH", 4)) {
        const char *a = at + 4;
        uint8_t n = (uint8_t)strlen(a);
        if (n == 0) {
            fcHeader = 0;
            fcHeaderExt = 0;
        } else {
            if ((n != 3 && n != 6 && n != 8) || !elm_is_hex_string(a) ||
                elm_parse_hex(a) > (n == 3   ? 0x7ffU
                                    : n == 8 ? 0x1fffffffU
                                             : 0xffffffU)) {
                elm_line("?");
                return;
            }
            uint32_t addr = elm_parse_hex(a);
            if (n > 3) {
                fcHeader = (n == 8) ? addr : (((uint32_t)canPriority << 24) | (addr & 0x00FFFFFF));
                fcHeaderExt = 1;
            } else {
                fcHeader = addr;
                fcHeaderExt = 0;
            }
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "FCSD", 4)) {
        const char *a = at + 4;
        uint8_t n = (uint8_t)strlen(a);
        if ((n && (n < 2 || n > 10 || (n & 1) || !elm_is_hex_string(a)))) {
            elm_line("?");
            return;
        }
        uint8_t k = 0;
        for (uint8_t i = 0; (i + 1) < n && k < 5; i += 2)
            fcData[k++] = (uint8_t)((elm_hexval(a[i]) << 4) | elm_hexval(a[i + 1]));
        if (k == 0) {
            fcData[0] = 0x30;
            fcData[1] = 0x00;
            fcData[2] = 0x00;
            k = 3;
        }
        fcDataLen = k;
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "FCSM", 4)) {
        if (strlen(at) != 5 || at[4] < '0' || at[4] > '2') {
            elm_line("?");
            return;
        }
        uint8_t v = (uint8_t)(at[4] - '0');
        fcMode = (v <= 2) ? v : 0;
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "CF", 2)) {
        const char *a = at + 2;
        if (strlen(a) == 0) {
            canFilterValue = 0;
            canFilterMask = 0;
        } else {
            canFilterValue = elm_parse_hex(a);
            if (canFilterMask == 0)
                canFilterMask = (strlen(a) > 3) ? 0x1FFFFFFF : 0x7FF;
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "CM", 2)) {
        const char *a = at + 2;
        canFilterMask = (strlen(a) == 0) ? 0 : elm_parse_hex(a);
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "PP", 2)) {
        const char *a = at + 2;
        if (strlen(a) < 2) {
            elm_line("?");
            return;
        }
        if (elm_hexval(a[0]) != 0xFF && elm_hexval(a[1]) != 0xFF) {
            uint8_t pn = (uint8_t)((elm_hexval(a[0]) << 4) | elm_hexval(a[1]));
            const char *rest = a + 2;
            if (!strncmp(rest, "SV", 2)) {
                if (strlen(rest) != 4 || !elm_is_hex_string(rest + 2)) {
                    elm_line("?");
                    return;
                }
                uint8_t v = (uint8_t)((elm_hexval(rest[2]) << 4) | elm_hexval(rest[3]));
                switch (pn) {
                case 0x2C:
                    pp2C = v;
                    break;
                case 0x2D:
                    pp2D = v;
                    break;
                case 0x2E:
                    pp2E = v;
                    break;
                case 0x2F:
                    pp2F = v;
                    break;
                default:
                    break;
                }
            }
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "SP", 2) || !strncmp(at, "TP", 2)) {
        const char *a = at + 2;
        if (*a == 'A') {
            protoAuto = 1;
            a++;
        } else
            protoAuto = 0;
        uint8_t v = elm_hexval(*a);
        if (v == 0xFF) {
            elm_line("OK");
            return;
        }
        if (v == 0) {
            protoAuto = 1;
        } else {
            protoNum = v;
        }
        switch (protoNum) {
        case 6:
        case 7:
            elm_apply_bitrate_divisor(1);
            break;
        case 8:
        case 9:
            elm_apply_bitrate_divisor(2);
            break;
        case 0xA:
            elm_apply_bitrate_divisor(2);
            break;
        case 0xB:
            elm_apply_bitrate_divisor(pp2D);
            break;
        case 0xC:
            elm_apply_bitrate_divisor(pp2F);
            break;
        default:
            break;
        }
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "IB", 2) || !strncmp(at, "FC", 2) || !strncmp(at, "BRT", 3)) {
        elm_line("OK");
        return;
    }
    if (!strncmp(at, "BRD", 3)) {
        elm_puts("OK");
        elm_eol();
        elm_flush();
        HAL_Delay(15);
        elm_puts(ELM327_ID_STRING);
        elm_eol();
        elm_flush();
        uint32_t t0 = currentTime;
        while (currentTime - t0 < 2000) {
            cdc_process();
            int16_t c = elm_ring_get();
            if (c < 0)
                continue;
            if (c == '\r' || c == '\n')
                break;
        }
        while (elm_ring_get() >= 0)
            ;
        cmdLen = 0;
        elm_puts("OK");
        elm_eol();
        return;
    }
    #ifndef ELM327_TRACE_DISABLE
    if (!strcmp(at, "LOG")) {
        traceDumping = 1;
        elm_line("---- trace ----");
        if (traceWrapped) {
            for (uint16_t i = traceHead; i < ELM327_TRACE_LEN; i++)
                elm_putc(traceBuf[i]);
        }
        for (uint16_t i = 0; i < traceHead; i++)
            elm_putc(traceBuf[i]);
        elm_eol();
        elm_line("---- end trace ----");
        traceDumping = 0;
        return;
    }
    if (!strcmp(at, "LOGC")) {
        traceHead = 0;
        traceWrapped = 0;
        elm_line("OK");
        return;
    }
    #endif
    #ifdef ELM327_UNKNOWN_AT_IS_ERROR
    elm_line("?");
    #else
    elm_line("OK");
    #endif
}
/* Exchange raw CAN diagnostic frames using the client-selected formatting. */
static uint8_t elm_handle_obd_raw(const uint8_t *payload, uint8_t payloadLen, uint8_t expectedResponses) {
    if (!elm_bus_send(payload, payloadLen, canSendHeader, extendedHeader)) {
        return 0;
    }
    CAN_RxHeaderTypeDef resp;
    uint8_t rd[8];
    uint8_t printed = 0;
    uint8_t sawPending = 0;
    uint32_t t0 = currentTime;
    uint32_t start = currentTime;
    uint16_t waitMs = elm_wait_budget();
    while ((currentTime - t0 < waitMs) && (currentTime - start < (uint32_t)waitMs * 4 + 100) &&
           (printed < ELM327_MAX_RAW_FRAMES)) {
        elm_bus_pump();
        if (elm_remote_finished())
            break;
        if (!elm_can_get_frame(&resp, rd))
            continue;
        status_led_activity();
        elm_print_header(&resp);
        for (uint8_t i = 0; i < resp.DLC && i < 8; i++) {
            if (spacesOn && i > 0)
                elm_putc(' ');
            elm_puthex(rd[i]);
        }
        elm_eol();
        elm_flush();
        printed++;
        if ((rd[0] & 0xF0) == 0x10 && cfcOn)
            elm_send_flow_control(&resp);
        if (expectedResponses && printed >= expectedResponses) {
            elm_remote_drain();
            return 1;
        }
        if ((rd[0] & 0xF0) == 0x00) {
            if (resp.DLC >= 4 && rd[1] == 0x7F && rd[3] == 0x78)
                sawPending = 1;
            else if (sawPending) {
                elm_remote_drain();
                return 1;
            }
        }
        t0 = currentTime;
    }
    elm_remote_drain();
    return printed ? 1 : 0;
}
/* Exchange diagnostic payloads and assemble complete multipart controller replies. */
static uint8_t elm_handle_obd_caf(const uint8_t *payload, uint16_t payloadLen, uint8_t expectedResponses) {
    uint32_t transaction_started = currentTime;
    (void)expectedResponses;
    uint8_t frame[8];
    if (payloadLen <= 7) {
        memset(frame, ELM327_PAD_BYTE, sizeof(frame));
        frame[0] = (uint8_t)payloadLen;
        memcpy(&frame[1], payload, payloadLen);
        uint8_t dlc = variableDlc ? (uint8_t)(payloadLen + 1) : 8;
        if (!elm_bus_send(frame, dlc, canSendHeader, extendedHeader)) {
            return 0;
        }
    } else {
        memset(frame, ELM327_PAD_BYTE, sizeof(frame));
        frame[0] = (uint8_t)(0x10 | ((payloadLen >> 8) & 0x0F));
        frame[1] = (uint8_t)(payloadLen & 0xFF);
        memcpy(&frame[2], payload, 6);
        if (!elm_can_send_frame(frame)) {
            return 0;
        }
        uint8_t blockSize = 0, stMin = 0;
        uint8_t gotFC = 0;
        CAN_RxHeaderTypeDef h;
        uint8_t d[8];
        uint32_t t0 = currentTime;
        while (currentTime - t0 < ELM327_FC_TIMEOUT_MS && currentTime - transaction_started < 10000) {
            elm_bus_pump();
            if (!elm_can_get_frame(&h, d))
                continue;
            if (h.DLC < 3 || (d[0] & 0xF0) != 0x30)
                continue;
            if ((d[0] & 0x0F) == 1) {
                t0 = currentTime;
                continue;
            }
            if ((d[0] & 0x0F) == 2) {
                return 0;
            }
            blockSize = d[1];
            stMin = d[2];
            gotFC = 1;
            break;
        }
        if (!gotFC) {
            return 0;
        }
        if (stMin > 127)
            stMin = 1;
        uint16_t sent = 6;
        uint8_t sn = 1;
        uint8_t inBlock = 0;
        while (sent < payloadLen) {
            uint8_t toCopy = (uint8_t)((payloadLen - sent) > 7 ? 7 : (payloadLen - sent));
            memset(frame, ELM327_PAD_BYTE, sizeof(frame));
            frame[0] = (uint8_t)(0x20 | (sn & 0x0F));
            memcpy(&frame[1], &payload[sent], toCopy);
            if (!elm_can_send_frame(frame)) {
                return 0;
            }
            sent += toCopy;
            sn++;
            if (stMin)
                HAL_Delay(stMin);
            if (blockSize) {
                inBlock++;
                if (inBlock >= blockSize && sent < payloadLen) {
                    inBlock = 0;
                    uint8_t again = 0;
                    uint32_t tf = currentTime;
                    while (currentTime - tf < ELM327_FC_TIMEOUT_MS &&
                           currentTime - transaction_started < 10000) {
                        elm_bus_pump();
                        if (!elm_can_get_frame(&h, d))
                            continue;
                        if (h.DLC < 3 || (d[0] & 0xF0) != 0x30)
                            continue;
                        if ((d[0] & 0x0F) == 1) {
                            tf = currentTime;
                            continue;
                        }
                        if ((d[0] & 0x0F) == 2) {
                            return 0;
                        }
                        blockSize = d[1];
                        stMin = (d[2] > 127) ? 1 : d[2];
                        again = 1;
                        break;
                    }
                    if (!again) {
                        return 0;
                    }
                }
            }
        }
    }
    static uint8_t isoData[ELM327_ISOTP_MAX_LEN];
    uint16_t isoTotal = 0;
    uint16_t isoReceived = 0;
    uint32_t iso_id = 0;
    uint8_t iso_ext = 0;
    uint8_t isoStarted = 0;
    uint8_t isoSN = 1;
    CAN_RxHeaderTypeDef resp;
    uint8_t rd[8];
    uint8_t answered = 0;
    uint32_t t0 = currentTime;
    uint16_t waitMs = elm_wait_budget();
    while (currentTime - t0 < waitMs && currentTime - transaction_started < 10000) {
        elm_bus_pump();
        if (elm_remote_finished())
            break;
        if (!elm_can_get_frame(&resp, rd))
            continue;
        if (isoStarted &&
            (resp.IDE != iso_ext || (resp.IDE == CAN_ID_EXT ? resp.ExtId : resp.StdId) != iso_id))
            continue;
        uint8_t pci = rd[0];
        if ((pci & 0xF0) == 0x00) {
            uint8_t len = pci & 0x0F;
            if (len > 7 || len + 1U > resp.DLC)
                continue;
            status_led_activity();
            elm_print_header(&resp);
            for (uint8_t i = 1; i <= len; i++) {
                if (spacesOn && i > 1)
                    elm_putc(' ');
                elm_puthex(rd[i]);
            }
            elm_eol();
            elm_flush();
            if (len >= 3 && rd[1] == 0x7F && rd[3] == 0x78) {
                answered = 1;
                t0 = currentTime;
                continue;
            }
            elm_remote_drain();
            return 1;
        } else if ((pci & 0xF0) == 0x10) {
            if (resp.DLC != 8)
                return 0;
            isoTotal = (uint16_t)(((uint16_t)(pci & 0x0F) << 8) | rd[1]);
            if (isoTotal <= 7 || isoTotal > ELM327_ISOTP_MAX_LEN)
                return 0;
            isoReceived = 0;
            isoStarted = 1;
            iso_id = resp.IDE == CAN_ID_EXT ? resp.ExtId : resp.StdId;
            iso_ext = resp.IDE;
            isoSN = 1;
            uint8_t toCopy = (uint8_t)(isoTotal < 6 ? isoTotal : 6);
            for (uint8_t i = 0; i < toCopy; i++)
                isoData[isoReceived++] = rd[2 + i];
            if (cfcOn)
                elm_send_flow_control(&resp);
            t0 = currentTime;
        } else if ((pci & 0xF0) == 0x20 && isoStarted) {
            if (resp.IDE != iso_ext || (resp.IDE == CAN_ID_EXT ? resp.ExtId : resp.StdId) != iso_id)
                continue;
            if ((pci & 0x0F) != (isoSN & 0x0F)) {
                return 0;
            }
            isoSN++;
            uint16_t remaining = (uint16_t)(isoTotal - isoReceived);
            uint8_t toCopy = (uint8_t)(remaining > 7 ? 7 : remaining);
            if (resp.DLC < toCopy + 1U)
                return 0;
            for (uint8_t i = 0; i < toCopy && isoReceived < ELM327_ISOTP_MAX_LEN; i++)
                isoData[isoReceived++] = rd[1 + i];
            t0 = currentTime;
            if (isoReceived >= isoTotal) {
                status_led_activity();
                elm_print_header(&resp);
                for (uint16_t i = 0; i < isoReceived; i++) {
                    if (spacesOn && i > 0)
                        elm_putc(' ');
                    elm_puthex(isoData[i]);
                }
                elm_eol();
                elm_flush();
                elm_remote_drain();
                return 1;
            }
        }
    }
    elm_remote_drain();
    return answered;
}
/* Locate the requested controller and perform its diagnostic exchange. */
static void elm_handle_obd(const char *hexCmd) {
    uint8_t payload[32];
    uint16_t payloadLen = 0;
    uint16_t l = (uint16_t)strlen(hexCmd);
    uint8_t expectedResponses = 0;
    if (l & 1) {
        if (!cafOn) {
            elm_line("?");
            return;
        }
        expectedResponses = elm_hexval(hexCmd[l - 1]);
        if (expectedResponses == 0xFF)
            expectedResponses = 0;
        l--;
    }
    for (uint16_t i = 0; (i + 1) < l && payloadLen < sizeof(payload); i += 2) {
        payload[payloadLen++] = (uint8_t)((elm_hexval(hexCmd[i]) << 4) | elm_hexval(hexCmd[i + 1]));
    }
    if (payloadLen == 0 || l > sizeof(payload) * 2) {
        elm_line("?");
        return;
    }
    uint32_t savedFilter = canFilterValue, savedMask = canFilterMask;
    if (!canFilterMask) {
        if (extendedHeader) {
            canFilterValue = 0x18daf100U | ((canSendHeader >> 8) & 0xff);
            canFilterMask = 0x1fffffffU;
            if ((canSendHeader & 0x1fff0000U) == 0x18db0000U) {
                canFilterValue = 0x18daf100U;
                canFilterMask = 0x1fffff00U;
            }
        } else {
            canFilterValue = canSendHeader == 0x7df ? 0x7e8 : canSendHeader + 8;
            canFilterMask = canSendHeader == 0x7df ? 0x7f8 : 0x7ff;
        }
    }
    #if defined(BACCABLE_C1)
    uint8_t order[ELMLINK_BUS_COUNT];
    uint8_t candidates = elm_bus_order(order);
    uint8_t probing = (candidates > 1);
    for (uint8_t k = 0; k < candidates; k++) {
        activeBus = order[k];
        if (activeBus == ELMLINK_BUS_LOCAL &&
            can_set_receive_filter(canFilterValue, canFilterMask, extendedHeader) != HAL_OK)
            continue;
        rspTimeout = probing ? ELM327_PROBE_TIMEOUT_MS : cmdTimeout;
        if (rspTimeout > cmdTimeout)
            rspTimeout = cmdTimeout;
        trace_str(activeBus == ELMLINK_BUS_LOCAL ? "{C1}" : (activeBus == ELMLINK_BUS_C2 ? "{C2}" : "{BH}"));
        if (activeBus != ELMLINK_BUS_LOCAL) {
            elm_remote_clear();
            if (!elmlink_send_config(activeBus, canFilterValue, canFilterMask, rspTimeout, cfcOn))
                continue;
            if (fcMode && fcDataLen &&
                !elmlink_send_fc_config(activeBus, fcHeader ? fcHeader : canSendHeader,
                                        fcHeader ? fcHeaderExt : extendedHeader, fcData, fcDataLen))
                continue;
        }
        uint8_t answered = cafOn ? elm_handle_obd_caf(payload, payloadLen, expectedResponses)
                                 : elm_handle_obd_raw(payload, (uint8_t)payloadLen, expectedResponses);
        if (activeBus != ELMLINK_BUS_LOCAL && remoteFailed) {
            canFilterValue = savedFilter;
            canFilterMask = savedMask;
            activeBus = ELMLINK_BUS_LOCAL;
            rspTimeout = cmdTimeout;
            elm_line("BUFFER FULL");
            return;
        }
        if (answered) {
            elm_route_store(elm_target_addr(), activeBus);
            canFilterValue = savedFilter;
            canFilterMask = savedMask;
            activeBus = ELMLINK_BUS_LOCAL;
            rspTimeout = cmdTimeout;
            return;
        }
    }
    activeBus = ELMLINK_BUS_LOCAL;
    rspTimeout = cmdTimeout;
    canFilterValue = savedFilter;
    canFilterMask = savedMask;
    elm_line("NO DATA");
    #else
    rspTimeout = cmdTimeout;
    uint8_t answered = cafOn ? elm_handle_obd_caf(payload, payloadLen, expectedResponses)
                             : elm_handle_obd_raw(payload, (uint8_t)payloadLen, expectedResponses);
    if (!answered)
        elm_line("NO DATA");
    #endif
}
/* Dispatch a complete client command and finish its response with a prompt. */
static void elm_execute(char *cmd) {
    trace_str("\r\n<");
    trace_str(cmd);
    trace_str(">");
    if (echoOn) {
        elm_puts(cmd);
        elm_eol();
    }
    if (cmd[0] == 'A' && cmd[1] == 'T') {
        elm_handle_at(cmd + 2);
    } else if (elm_is_hex_string(cmd)) {
        elm_handle_obd(cmd);
    } else {
        elm_line("?");
    }
    #ifdef ELM327_EXTRA_CR_BEFORE_PROMPT
    elm_putc('\r');
    #endif
    elm_putc('>');
    elm_flush();
}
/* Process complete diagnostic commands and expire inactive sessions. */
void elm327_process(void) {
    int16_t c;
    #if defined(BACCABLE_C1)
    if (!elmModeOn)
        return;
    if ((currentTime - elmLastCmd) > ELM327_IDLE_EXIT_MS) {
        elm327_set_enabled(0);
        return;
    }
    #endif
    if (cmdLen && (currentTime - lastRxByteTime) > 1000) {
        cmdLen = 0;
        discardCommand = 1;
    }
    while ((c = elm_ring_get()) >= 0) {
        lastRxByteTime = currentTime;
    #if defined(BACCABLE_C1)
        elmLastCmd = currentTime;
    #endif
        if (c == '\r' || c == '\n') {
            if (discardCommand) {
                discardCommand = 0;
                cmdLen = 0;
                outputFailed = 0;
                elm_line("?");
                elm_putc('>');
                elm_flush();
                return;
            }
            if (cmdLen == 0)
                continue;
            cmdBuf[cmdLen] = '\0';
            cmdLen = 0;
            outputFailed = 0;
            elm_execute(cmdBuf);
            return;
        }
        if (discardCommand)
            continue;
        if (c == ' ')
            continue;
        if (c < 0x20 || c >= 0x7F)
            continue;
        if (cmdLen >= (ELM327_CMD_BUF_LEN - 1)) {
            discardCommand = 1;
            cmdLen = 0;
            continue;
        }
        if (c >= 'a' && c <= 'z')
            c = (int16_t)(c - 'a' + 'A');
        cmdBuf[cmdLen++] = (char)c;
    }
}
#endif
