#ifndef BACCABLE_DISPLAY_STREAM_H
#define BACCABLE_DISPLAY_STREAM_H
#include "app/build_config.h"
#include <stdbool.h>
#include <stdint.h>
/* Main-loop owned. A view is immutable until all of its fragments are accepted. */
typedef struct {
    uint8_t active[DASHBOARD_MESSAGE_MAX_LENGTH];
    uint8_t pending[DASHBOARD_MESSAGE_MAX_LENGTH];
    uint8_t fragment;
    bool sending, queued, valid;
} DisplayStream;
void display_stream_submit(DisplayStream *stream, const uint8_t *text);
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]);
void display_stream_accept(DisplayStream *stream);
void display_stream_reset(DisplayStream *stream);
void display_stream_refresh(DisplayStream *stream);
#endif
