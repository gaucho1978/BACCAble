#ifndef BACCABLE_PROTOCOL_SLCAN_CODEC_H
#define BACCABLE_PROTOCOL_SLCAN_CODEC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define SLCAN_TEXT_CAPACITY 28
typedef struct {
    uint32_t id;
    uint8_t length;
    bool extended;
    bool remote;
    uint8_t data[8];
} CanFrame;
bool can_frame_valid(const CanFrame *frame);
bool slcan_decode(const uint8_t *text, size_t length, CanFrame *frame);
size_t slcan_encode(const CanFrame *frame, uint8_t *text, size_t capacity);
#endif
