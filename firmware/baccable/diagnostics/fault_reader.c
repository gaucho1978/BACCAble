/* UDS fault reading adapted from gaucho1978 BACCAble, August 2026. */
#include "diagnostics/fault_reader.h"
#include "features/ui_entry.h"
#if defined(BACCABLE_C1)
    #include "platform/system.h"
    #include "transport/can_bus.h"
    #include "third_party/printf/printf.h"
    #include <string.h>
    #define FAULT_LIMIT 20
static enum { IDLE, START, SESSION, QUERY, RESPONSE, FLOW, FRAGMENTS, DONE, FAILED } state;
static uint8_t ecu_address, sequence, count;
static uint16_t expected, received;
static uint32_t started, updated;
static uint8_t payload[3 + 4 * FAULT_LIMIT];
static const char *failure;

/* Keep the reason for a stopped read until the user retries or leaves. */
static void fail(const char *reason) {
    failure = reason;
    state = FAILED;
}

/* Distinguish a request that could not be queued from an overdue response. */
static void fail_timeout(void) {
    fail(state == START || state == QUERY || state == FLOW ? UI_SYMBOL_WARNING " CAN send failed" : UI_SYMBOL_WARNING " Read timeout");
}

/* Begin reading stored and current faults from the selected controller. */
void fault_reader_start(uint8_t ecu) {
    ecu_address = ecu;
    state = START;
    failure = NULL;
    count = received = expected = 0;
    started = updated = currentTime;
}

/* Stop a read when the user leaves its screen. */
void fault_reader_cancel(void) { state = IDLE; }

/* Report whether this reader still owns a diagnostic transaction. */
bool fault_reader_busy(void) { return state >= START && state <= FRAGMENTS; }

/* Return the number of complete fault entries available for browsing. */
unsigned fault_reader_count(void) { return state == DONE ? count : 0; }

/* Queue a diagnostic request without advancing when the CAN queue is full. */
static bool send(const uint8_t *data, uint8_t length) {
    CAN_TxHeaderTypeDef header = {.ExtId = 0x18da00f1U | (uint32_t)ecu_address << 8,
                                  .IDE = CAN_ID_EXT,
                                  .RTR = CAN_RTR_DATA,
                                  .DLC = length};
    return can_tx(&header, (uint8_t *)data) == HAL_OK;
}

/* Advance session setup and flow control while bounding missing or pending replies. */
void fault_reader_process(void) {
    if (!fault_reader_busy())
        return;
    if (currentTime - updated >= 2000 || currentTime - started >= 10000) {
        fail_timeout();
        return;
    }
    const uint8_t session[] = {2, 0x10, 3};
    const uint8_t query[] = {3, 0x19, 2, 0xff};
    const uint8_t flow[] = {0x30, 0, 5};
    bool sent = false;
    if (state == START && send(session, sizeof(session))) {
        state = SESSION;
        sent = true;
    } else if (state == QUERY && send(query, sizeof(query))) {
        state = RESPONSE;
        sent = true;
    } else if (state == FLOW && send(flow, sizeof(flow))) {
        state = FRAGMENTS;
        sent = true;
    }
    if (sent)
        updated = currentTime;
}

/* Retain the first twenty complete faults while consuming the full response safely. */
static void append(const uint8_t *data, unsigned length) {
    for (unsigned i = 0; i < length && received < expected; ++i, ++received)
        if (received < sizeof(payload))
            payload[received] = data[i];
    updated = currentTime;
    if (received == expected) {
        if (payload[0] != 0x59 || payload[1] != 2 || expected < 3 || (expected - 3) % 4)
            fail(UI_SYMBOL_WARNING " Invalid reply");
        else {
            count = (expected - 3) / 4 > FAULT_LIMIT ? FAULT_LIMIT : (expected - 3) / 4;
            state = DONE;
        }
    }
}

/* Accept only complete, ordered replies from the controller being read. */
void fault_reader_receive(const CAN_RxHeaderTypeDef *h, const uint8_t *d) {
    if (!fault_reader_busy() || h->IDE != CAN_ID_EXT || h->RTR != CAN_RTR_DATA ||
        h->ExtId != (0x18daf100U | ecu_address) || h->DLC < 2 || h->DLC > 8)
        return;
    if (currentTime - updated >= 2000 || currentTime - started >= 10000) {
        fail_timeout();
        return;
    }
    if (d[0] == 3 && h->DLC >= 4 && d[1] == 0x7f && d[2] == (state == SESSION ? 0x10 : 0x19)) {
        if (d[3] == 0x78)
            updated = currentTime;
        else
            fail(UI_SYMBOL_WARNING " ECU rejected");
        return;
    }
    if (state == SESSION) {
        if (d[0] >= 2 && d[0] <= 7 && d[0] + 1U <= h->DLC && d[1] == 0x50 && d[2] == 3) {
            state = QUERY;
            updated = currentTime;
        }
    } else if (state == RESPONSE && d[0] <= 7) {
        if (d[0] < 3 || d[0] + 1U > h->DLC || d[1] != 0x59 || d[2] != 2)
            return;
        expected = d[0];
        received = 0;
        append(d + 1, d[0]);
    } else if (state == RESPONSE && (d[0] >> 4) == 1) {
        if (h->DLC != 8 || d[2] != 0x59 || d[3] != 2)
            return;
        expected = (uint16_t)(d[0] & 15) << 8 | d[1];
        if (expected <= 7 || (expected - 3) % 4) {
            fail(UI_SYMBOL_WARNING " Invalid reply");
            return;
        }
        received = 0;
        sequence = 1;
        append(d + 2, 6);
        state = FLOW;
    } else if (state == FRAGMENTS && (d[0] >> 4) == 2) {
        if ((d[0] & 15) != sequence || (expected - received > 7 && h->DLC != 8) ||
            h->DLC - 1 < (unsigned)(expected - received < 7 ? expected - received : 7)) {
            fail(UI_SYMBOL_WARNING " Invalid reply");
            return;
        }
        sequence = (sequence + 1) & 15;
        append(d + 1, h->DLC - 1);
    }
}

/* Show progress, failure or a fault code with its position in the result list. */
void fault_reader_text(unsigned index, char *text, size_t capacity) {
    if (state != DONE || !count) {
        snprintf_(text, capacity, "%s",
                  fault_reader_busy() ? "Reading faults..."
                  : state == DONE     ? "No faults reported"
                  : state == FAILED   ? failure
                                      : "Read faults: RES");
        return;
    }
    index %= count;
    const uint8_t *d = payload + 3 + index * 4;
    char code[16];
    snprintf_(code, sizeof(code), "%s%c%01X%01X%02X-%02X", expected > sizeof(payload) ? "+ " : "",
              "PCBU"[d[0] >> 6], (d[0] >> 4) & 3, d[0] & 15, d[1], d[2]);
    ui_render_list_entry(text, capacity, index + 1, count, code);
}
#endif
