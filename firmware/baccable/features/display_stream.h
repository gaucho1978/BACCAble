#ifndef BACCABLE_DISPLAY_STREAM_H
#define BACCABLE_DISPLAY_STREAM_H
#include "app/build_config.h"
#include <stdbool.h>
#include <stdint.h>
#define DISPLAY_FRAGMENT_SIZE 3U
#define DISPLAY_FRAGMENT_COUNT (DASHBOARD_MESSAGE_MAX_LENGTH / DISPLAY_FRAGMENT_SIZE)
_Static_assert(DASHBOARD_MESSAGE_MAX_LENGTH % DISPLAY_FRAGMENT_SIZE == 0 && DISPLAY_FRAGMENT_COUNT <= 8,
               "Display fragments must fit the tracking mask");
/* Main-loop owned. Sent text records CAN queue acceptance, not an acknowledgment from the IPC. */
typedef struct {
    uint8_t sent[DASHBOARD_MESSAGE_MAX_LENGTH];
    uint8_t target[DASHBOARD_MESSAGE_MAX_LENGTH];
    uint8_t offered[DISPLAY_FRAGMENT_SIZE];
    uint8_t known, forced, dirty, cursor, fragment;
    bool valid, offering;
} DisplayStream;
void display_stream_submit(DisplayStream *stream, const uint8_t *text);
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]);
void display_stream_accept(DisplayStream *stream);
void display_stream_reset(DisplayStream *stream);
void display_stream_refresh(DisplayStream *stream);
#endif
