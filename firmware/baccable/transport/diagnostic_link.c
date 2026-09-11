/* Adapted from gaucho1978 BACCAble 02b2fd8; see LICENSE.MD and UPSTREAM_SYNC.md. */

#include "transport/diagnostic_link.h"
#if defined(BACCABLE_C1) || defined(BACCABLE_C2) || defined(BACCABLE_BH)
    #include <string.h>
    #include "app/application_state.h"
    #include "transport/board_uart.h"
    #include "platform/status_led.h"
extern UART_HandleTypeDef huart2;
static uint8_t linkEnabled = 0;
static uint8_t seqCounter = 0;
static uint32_t last_activity;
static uint8_t inbox_lost;
    #define ELMLINK_INBOX_LEN 4
static volatile uint8_t inbox[ELMLINK_INBOX_LEN][UART_BUFFER_SIZE];
static volatile uint8_t inboxHead = 0, inboxTail = 0;
/* Keep an incoming board message until the diagnostic task can handle it. */
static uint8_t inbox_push(const uint8_t *f) {
    uint8_t next = (uint8_t)((inboxHead + 1) % ELMLINK_INBOX_LEN);
    if (next == inboxTail) {
        inbox_lost = 1;
        return 0;
    }
    memcpy((void *)inbox[inboxHead], f, UART_BUFFER_SIZE);
    inboxHead = next;
    return 1;
}
/* Take the next waiting diagnostic message from another board. */
static uint8_t inbox_pop(uint8_t *out) {
    if (inboxTail == inboxHead)
        return 0;
    memcpy(out, (const void *)inbox[inboxTail], UART_BUFFER_SIZE);
    inboxTail = (uint8_t)((inboxTail + 1) % ELMLINK_INBOX_LEN);
    return 1;
}
/* Discard board messages that belong to an earlier exchange. */
static void inbox_clear(void) {
    inboxTail = inboxHead;
    inbox_lost = 0;
}
/* Give diagnostics ownership of the board link or restore normal vehicle features. */
void elmlink_set_enabled(uint8_t on) {
    linkEnabled = on ? 1 : 0;
    last_activity = currentTime;
    if (!on) {
        inbox_clear();
        can_set_receive_filter(0, 0, 0);
    }
    board_uart_set_diagnostic(on);
}
/* Report whether the board link is reserved for diagnostics. */
uint8_t elmlink_is_enabled(void) { return linkEnabled; }
/* Detect damaged diagnostic messages between boards. */
static uint8_t elmlink_checksum(const uint8_t *f) {
    uint8_t x = 0;
    for (uint8_t i = 0; i < 17; i++)
        x = (uint8_t)(x ^ f[i]);
    return x;
}
/* Prepare a diagnostic request or reply for the destination board. */
static void elmlink_build(uint8_t *f, uint8_t dest, uint8_t type, uint8_t flags, uint32_t id,
                          const uint8_t *data, uint8_t dlc, uint8_t seq) {
    memset(f, 0x20, UART_BUFFER_SIZE);
    f[0] = dest;
    f[1] = type;
    f[2] = flags;
    f[3] = (uint8_t)((id >> 24) & 0xFF);
    f[4] = (uint8_t)((id >> 16) & 0xFF);
    f[5] = (uint8_t)((id >> 8) & 0xFF);
    f[6] = (uint8_t)(id & 0xFF);
    f[7] = dlc;
    memset(&f[8], 0, 8);
    if (data && dlc)
        memcpy(&f[8], data, dlc > 8 ? 8 : dlc);
    f[16] = seq;
    f[17] = elmlink_checksum(f);
}
/* Read the controller address carried by a board message. */
static uint32_t elmlink_id_of(const uint8_t *f) {
    return ((uint32_t)f[3] << 24) | ((uint32_t)f[4] << 16) | ((uint32_t)f[5] << 8) | (uint32_t)f[6];
}
/* Deliver one complete diagnostic message to another board. */
static uint8_t elmlink_send(const uint8_t *f) { return board_uart_diagnostic_send(f, UART_BUFFER_SIZE); }
/* Validate and route diagnostic messages received from another board. */
uint8_t elmlink_on_uart_frame(const uint8_t *frame) {
    uint8_t dest = frame[0];
    if (dest != ELMLINK_TO_C2 && dest != ELMLINK_TO_BH && dest != ELMLINK_TO_MASTER)
        return 0;
    if (elmlink_checksum(frame) != frame[17] || frame[7] > 8)
        return 2;
    if ((frame[1] == ELMLINK_TYPE_REQ || frame[1] == ELMLINK_TYPE_RSP) &&
        elmlink_id_of(frame) > ((frame[2] & ELMLINK_FLAG_EXTID) ? 0x1fffffffU : 0x7ffU))
        return 2;
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (dest == ELMLINK_SLAVE_ID && frame[1] == ELMLINK_TYPE_ARM) {
        elmlink_set_enabled((frame[2] & ELMLINK_FLAG_ARM_ON) != 0);
        if (!linkEnabled)
            inbox_clear();
        return 1;
    }
    #endif
    if (!linkEnabled)
        return 1;
    #if defined(BACCABLE_C1)
    if (dest != ELMLINK_TO_MASTER)
        return 1;
    #endif
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (dest != ELMLINK_SLAVE_ID)
        return 1;
    #endif
    last_activity = currentTime;
    inbox_push(frame);
    return 1;
}
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
static uint8_t slaveBusOpen = 0;
static uint8_t slaveCollecting = 0;
static uint32_t slaveDeadline = 0;
static uint8_t slaveSeq = 0;
static uint8_t slaveGotFrames = 0;
static uint8_t slaveLastPci = 0xFF;
static uint32_t slaveReqId = 0;
static uint8_t slaveReqExt = 0;
static uint8_t slaveAutoFc = 1;
static uint32_t slaveFcHeader = 0;
static uint8_t slaveFcExt = 0;
static uint8_t slaveFcData[8] = {0x30, 0x00, 0x0a, 0, 0, 0, 0, 0};
static uint8_t slaveFcLen = 3;
static uint32_t slaveFilterVal = 0;
static uint32_t slaveFilterMask = 0;
static uint16_t slaveTimeout = ELMLINK_DEFAULT_TIMEOUT_MS;
/* Make this auxiliary board's vehicle connection available for a request. */
static void slave_bus_open(void) {
    if (slaveBusOpen)
        return;
    can_enable();
    slaveBusOpen = 1;
}
/* Tell C1 that reply collection has finished or failed. */
static void slave_send_end(uint8_t noData) {
    uint8_t f[UART_BUFFER_SIZE];
    elmlink_build(f, ELMLINK_TO_MASTER, ELMLINK_TYPE_END, noData ? ELMLINK_FLAG_NODATA : 0, 0, NULL, 0,
                  slaveSeq);
    elmlink_send(f);
}
/* Apply C1's diagnostic settings or forward its request to a controller. */
static void slave_handle_frame(const uint8_t *f) {
    switch (f[1]) {
    case ELMLINK_TYPE_FCCFG:
        slaveFcHeader = elmlink_id_of(f);
        slaveFcExt = (f[2] & ELMLINK_FLAG_EXTID) ? 1 : 0;
        slaveFcLen = (f[7] > 8) ? 8 : f[7];
        if (slaveFcLen == 0) {
            slaveFcLen = 3;
            slaveFcData[0] = 0x30;
            slaveFcData[1] = 0x00;
            slaveFcData[2] = 0x0a;
        } else {
            memcpy(slaveFcData, &f[8], slaveFcLen);
        }
        break;
    case ELMLINK_TYPE_CFG:
        slaveFcHeader = 0;
        slaveFcExt = 0;
        slaveFcLen = 3;
        memset(slaveFcData, 0, sizeof(slaveFcData));
        slaveFcData[0] = 0x30;
        slaveFcData[2] = 10;
        slaveAutoFc = (f[14] & ELMLINK_CFG_AUTOFC) ? 1 : 0;
        slaveFilterVal = elmlink_id_of(f);
        slaveFilterMask =
            ((uint32_t)f[8] << 24) | ((uint32_t)f[9] << 16) | ((uint32_t)f[10] << 8) | (uint32_t)f[11];
        slaveTimeout = (uint16_t)(((uint16_t)f[12] << 8) | f[13]);
        if (slaveTimeout == 0)
            slaveTimeout = ELMLINK_DEFAULT_TIMEOUT_MS;
        break;
    case ELMLINK_TYPE_REQ: {
        CAN_TxHeaderTypeDef h;
        uint8_t data[8];
        uint8_t dlc = f[7];
        if (dlc > 8)
            dlc = 8;
        memcpy(data, &f[8], 8);
        slave_bus_open();
        h.RTR = CAN_RTR_DATA;
        h.DLC = dlc;
        h.TransmitGlobalTime = DISABLE;
        if (f[2] & ELMLINK_FLAG_EXTID) {
            h.IDE = CAN_ID_EXT;
            h.ExtId = elmlink_id_of(f) & 0x1FFFFFFF;
            h.StdId = 0;
        } else {
            h.IDE = CAN_ID_STD;
            h.StdId = elmlink_id_of(f) & 0x7FF;
            h.ExtId = 0;
        }
        slaveSeq = f[16];
        slaveReqId = elmlink_id_of(f);
        slaveReqExt = (f[2] & ELMLINK_FLAG_EXTID) ? 1 : 0;
        if (can_set_receive_filter(slaveFilterVal, slaveFilterMask, slaveReqExt) != HAL_OK) {
            slave_send_end(1);
            break;
        }
        if (can_tx(&h, data) != HAL_OK) {
            slave_send_end(1);
            break;
        }
        for (uint8_t i = 0; i < 8; i++)
            can_process();
        slaveGotFrames = 0;
        slaveLastPci = 0xFF;
        slaveCollecting = 1;
        slaveDeadline = currentTime + slaveTimeout;
        status_led_activity();
        break;
    }
    default:
        break;
    }
}
/* Recognize a controller that has requested more time to answer. */
static uint8_t elmlink_is_response_pending(const uint8_t *d) {
    if ((d[0] & 0xF0) != 0x00)
        return 0;
    return (d[1] == 0x7F && d[3] == 0x78) ? 1 : 0;
}
/* Forward a matching controller reply and request remaining response parts. */
static uint8_t slave_offer_rx(const CAN_RxHeaderTypeDef *h, const uint8_t *d) {
    if (!slaveCollecting || h->DLC == 0 || h->DLC > 8)
        return 0;
    uint32_t id = (h->IDE == CAN_ID_EXT) ? h->ExtId : h->StdId;
    if (slaveFilterMask && ((id & slaveFilterMask) != (slaveFilterVal & slaveFilterMask)))
        return 0;
    uint8_t f[UART_BUFFER_SIZE];
    elmlink_build(f, ELMLINK_TO_MASTER, ELMLINK_TYPE_RSP, (h->IDE == CAN_ID_EXT) ? ELMLINK_FLAG_EXTID : 0, id,
                  d, (uint8_t)h->DLC, slaveSeq);
    if (!elmlink_send(f)) {
        slaveCollecting = 0;
        slave_send_end(1);
        return 0;
    }
    slaveGotFrames++;
    slaveLastPci = d[0];
    uint16_t gap = ELMLINK_GAP_AFTER_SF_MS;
    if ((slaveLastPci & 0xF0) == 0x10) {
        if (slaveAutoFc) {
            CAN_TxHeaderTypeDef fh;
            uint8_t fd[8];
            memset(fd, 0, sizeof(fd));
            memcpy(fd, slaveFcData, slaveFcLen);
            if (slaveFcLen >= 3 && (fd[2] < 10 || fd[2] > 127))
                fd[2] = 10;
            fh.RTR = CAN_RTR_DATA;
            fh.DLC = slaveFcLen;
            fh.TransmitGlobalTime = DISABLE;
            uint32_t response_id = h->IDE == CAN_ID_EXT ? h->ExtId : h->StdId;
            uint32_t automatic =
                h->IDE == CAN_ID_EXT
                    ? (response_id & 0x1fff0000U) | ((response_id & 0xff) << 8) | ((response_id >> 8) & 0xff)
                : response_id >= 8 ? response_id - 8
                                   : slaveReqId;
            uint32_t fid = slaveFcHeader ? slaveFcHeader : automatic;
            uint8_t fex = slaveFcHeader ? slaveFcExt : slaveReqExt;
            if (fex) {
                fh.IDE = CAN_ID_EXT;
                fh.ExtId = fid & 0x1FFFFFFF;
                fh.StdId = 0;
            } else {
                fh.IDE = CAN_ID_STD;
                fh.StdId = fid & 0x7FF;
                fh.ExtId = 0;
            }
            if (can_tx(&fh, fd) != HAL_OK) {
                slaveCollecting = 0;
                slave_send_end(1);
                return 0;
            }
            for (uint8_t i = 0; i < 8; i++)
                can_process();
            gap = ELMLINK_GAP_AFTER_CF_MS;
        } else {
            gap = ELMLINK_GAP_AFTER_FF_MS;
        }
    } else if ((slaveLastPci & 0xF0) == 0x20)
        gap = ELMLINK_GAP_AFTER_CF_MS;
    if (gap > slaveTimeout)
        gap = slaveTimeout;
        #ifndef ELMLINK_NO_PENDING_WAIT
    if (h->DLC >= 4 && elmlink_is_response_pending(d))
        gap = slaveTimeout;
        #endif
    slaveDeadline = currentTime + gap;
    return 1;
}
    #endif
    #if defined(BACCABLE_C1)

/* Choose the auxiliary board serving the requested vehicle bus. */
static uint8_t bus_to_dest(uint8_t bus) { return (bus == ELMLINK_BUS_BH) ? ELMLINK_TO_BH : ELMLINK_TO_C2; }
/* Tell both auxiliary boards to enter or leave diagnostic operation. */
void elmlink_send_arm(uint8_t on) {
    uint8_t f[UART_BUFFER_SIZE];
    uint8_t flags = on ? ELMLINK_FLAG_ARM_ON : 0;
    elmlink_build(f, ELMLINK_TO_C2, ELMLINK_TYPE_ARM, flags, 0, NULL, 0, ++seqCounter);
    elmlink_send(f);
    HAL_Delay(10);
    elmlink_build(f, ELMLINK_TO_BH, ELMLINK_TYPE_ARM, flags, 0, NULL, 0, ++seqCounter);
    elmlink_send(f);
    HAL_Delay(10);
}
/* Prepare an auxiliary board with the current response filter and timeout. */
uint8_t elmlink_send_config(uint8_t bus, uint32_t filterValue, uint32_t filterMask, uint16_t timeoutMs,
                            uint8_t autoFlowControl) {
    if (!linkEnabled)
        return 0;
    uint8_t f[UART_BUFFER_SIZE];
    elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_ARM, ELMLINK_FLAG_ARM_ON, 0, NULL, 0, seqCounter);
    if (!elmlink_send(f))
        return 0;
    elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_CFG, 0, filterValue, NULL, 0, seqCounter);
    f[14] = autoFlowControl ? ELMLINK_CFG_AUTOFC : 0;
    f[8] = (uint8_t)((filterMask >> 24) & 0xFF);
    f[9] = (uint8_t)((filterMask >> 16) & 0xFF);
    f[10] = (uint8_t)((filterMask >> 8) & 0xFF);
    f[11] = (uint8_t)(filterMask & 0xFF);
    f[12] = (uint8_t)((timeoutMs >> 8) & 0xFF);
    f[13] = (uint8_t)(timeoutMs & 0xFF);
    f[17] = elmlink_checksum(f);
    return elmlink_send(f);
}
/* Pass the client's multipart-response preferences to an auxiliary board. */
uint8_t elmlink_send_fc_config(uint8_t bus, uint32_t fcHeader, uint8_t fcExt, const uint8_t *fcData,
                               uint8_t fcLen) {
    if (!linkEnabled)
        return 0;
    uint8_t f[UART_BUFFER_SIZE];
    elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_FCCFG, fcExt ? ELMLINK_FLAG_EXTID : 0, fcHeader, fcData,
                  fcLen, seqCounter);
    return elmlink_send(f);
}
/* Start a new controller request on an auxiliary board. */
uint8_t elmlink_send_request(uint8_t bus, uint32_t canId, uint8_t ext, const uint8_t *data, uint8_t dlc) {
    if (!linkEnabled)
        return 0;
    uint8_t f[UART_BUFFER_SIZE];
    seqCounter++;
    inbox_clear();
    elmlink_build(f, bus_to_dest(bus), ELMLINK_TYPE_REQ, ext ? ELMLINK_FLAG_EXTID : 0, canId, data, dlc,
                  seqCounter);
    return elmlink_send(f);
}
/* Deliver a waiting controller reply or report completion of the remote exchange. */
uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc)) {
    board_uart_process();
    if (inbox_lost) {
        inbox_clear();
        return 2;
    }
    uint8_t local[UART_BUFFER_SIZE];
    if (!inbox_pop(local))
        return 0;
    if (local[16] != seqCounter)
        return 0;
    if (local[1] == ELMLINK_TYPE_RSP) {
        if (onFrame)
            onFrame(elmlink_id_of(local), (local[2] & ELMLINK_FLAG_EXTID) ? 1 : 0, &local[8], local[7]);
        return 0;
    }
    if (local[1] == ELMLINK_TYPE_END)
        return 1;
    return 0;
}
    #endif
/* Clear earlier diagnostic exchanges when the board starts. */
void elmlink_init(void) {
    inbox_clear();
    seqCounter = 0;
}
/* Serve diagnostic requests and restore normal operation when the master disappears. */
void elmlink_process(void) {
    #if !defined(BACCABLE_C1)
    if (linkEnabled && currentTime - last_activity > 150000)
        elmlink_set_enabled(0);
    #endif
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (!linkEnabled) {
        slaveCollecting = 0;
        slaveBusOpen = 0;
        return;
    }
    #endif
    if (!linkEnabled)
        return;
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (inbox_lost) {
        inbox_clear();
        slaveCollecting = 0;
        slave_send_end(1);
        elmlink_set_enabled(0);
        return;
    }
    {
        uint8_t local[UART_BUFFER_SIZE];
        while (inbox_pop(local))
            slave_handle_frame(local);
    }
    CAN_RxHeaderTypeDef h;
    uint8_t d[8] = {0};
    for (unsigned budget = 0; budget < 16 && is_can_msg_pending(CAN_RX_FIFO0); ++budget) {
        if (can_rx(&h, d) != HAL_OK)
            break;
        if (h.RTR == CAN_RTR_DATA && slaveCollecting)
            slave_offer_rx(&h, d);
    }
    can_process();
    if (slaveCollecting && (int32_t)(currentTime - slaveDeadline) >= 0) {
        slaveCollecting = 0;
        slave_send_end(slaveGotFrames ? 0 : 1);
    }
    #endif
}
#endif
