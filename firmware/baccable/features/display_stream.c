#include "features/display_stream.h"
#include <string.h>

/* Keep the newest requested screen while the current screen finishes sending. */
void display_stream_submit(DisplayStream *stream, const uint8_t *text) {
    if (stream->sending && !memcmp(stream->active, text, sizeof(stream->active))) {
        stream->queued = false;
        return;
    }
    memcpy(stream->pending, text, sizeof(stream->pending));
    stream->queued = true;
}

/* Provide the next part of the screen without losing it if sending fails. */
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]) {
    if (!stream->sending) {
        if (!stream->queued)
            return false;
        memcpy(stream->active, stream->pending, sizeof(stream->active));
        stream->queued = false;
        stream->sending = true;
        stream->valid = true;
        stream->fragment = 0;
    }
    *fragment = stream->fragment;
    memcpy(text, stream->active + 3 * stream->fragment, 3);
    return true;
}

/* Advance the screen after its current part has been accepted for sending. */
void display_stream_accept(DisplayStream *stream) {
    if (stream->sending && ++stream->fragment == DASHBOARD_MESSAGE_MAX_LENGTH / 3)
        stream->sending = false;
}

/* Discard screen content when BACCAble display output is suspended. */
void display_stream_reset(DisplayStream *stream) { memset(stream, 0, sizeof(*stream)); }

/* Restore the current screen after factory display traffic, without restarting an active update. */
void display_stream_refresh(DisplayStream *stream) {
    if (!stream->valid || stream->sending || stream->queued)
        return;
    for (unsigned i = 0; i < sizeof(stream->active); ++i) {
        if (stream->active[i] != ' ') {
            display_stream_submit(stream, stream->active);
            return;
        }
    }
}
