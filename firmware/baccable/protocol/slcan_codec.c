#include "protocol/slcan_codec.h"

/* Interpret one hexadecimal character from a host command. */
static int hex_value(uint8_t ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

/* Check that a host-supplied CAN frame has a supported identifier and length. */
bool can_frame_valid(const CanFrame *frame) {
    return frame && frame->length <= 8 && frame->id <= (frame->extended ? 0x1fffffffU : 0x7ffU);
}

/* Read a complete host CAN command and reject malformed input. */
bool slcan_decode(const uint8_t *text, size_t length, CanFrame *frame) {
    if (!text || !frame || length == 0)
        return false;
    CanFrame parsed = {0};
    switch (text[0]) {
    case 'T':
        parsed.extended = true;
        break;
    case 't':
        break;
    case 'R':
        parsed.extended = true;
        parsed.remote = true;
        break;
    case 'r':
        parsed.remote = true;
        break;
    default:
        return false;
    }
    size_t digits = parsed.extended ? 8 : 3;
    if (length < digits + 2)
        return false;
    for (size_t i = 1; i <= digits; ++i) {
        int nibble = hex_value(text[i]);
        if (nibble < 0)
            return false;
        parsed.id = (parsed.id << 4) | (uint32_t)nibble;
    }
    int dlc = hex_value(text[digits + 1]);
    if (dlc < 0 || dlc > 8)
        return false;
    parsed.length = (uint8_t)dlc;
    size_t payload = parsed.remote ? 0 : parsed.length;
    if (length != digits + 2 + payload * 2 || !can_frame_valid(&parsed))
        return false;
    for (size_t i = 0; i < payload; ++i) {
        int high = hex_value(text[digits + 2 + i * 2]);
        int low = hex_value(text[digits + 3 + i * 2]);
        if (high < 0 || low < 0)
            return false;
        parsed.data[i] = (uint8_t)((high << 4) | low);
    }
    *frame = parsed;
    return true;
}

/* Describe a CAN frame in the format expected by host applications. */
size_t slcan_encode(const CanFrame *frame, uint8_t *text, size_t capacity) {
    static const char hex[] = "0123456789ABCDEF";
    if (!can_frame_valid(frame) || !text)
        return 0;
    size_t digits = frame->extended ? 8 : 3;
    size_t payload = frame->remote ? 0 : frame->length;
    size_t length = digits + 3 + payload * 2;
    if (capacity < length)
        return 0;
    text[0] = frame->remote ? (frame->extended ? 'R' : 'r') : (frame->extended ? 'T' : 't');
    for (size_t i = 0; i < digits; ++i)
        text[1 + i] = (uint8_t)hex[(frame->id >> (4 * (digits - i - 1))) & 15];
    text[digits + 1] = (uint8_t)hex[frame->length];
    for (size_t i = 0; i < payload; ++i) {
        text[digits + 2 + i * 2] = (uint8_t)hex[frame->data[i] >> 4];
        text[digits + 3 + i * 2] = (uint8_t)hex[frame->data[i] & 15];
    }
    text[length - 1] = '\r';
    return length;
}
