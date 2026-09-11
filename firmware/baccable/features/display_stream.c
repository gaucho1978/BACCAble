#include "features/display_stream.h"
#include <string.h>

/* Identify fragments that still need to reach the requested screen. */
static void update_dirty(DisplayStream *stream) {
    stream->dirty = stream->forced;
    for (unsigned i = 0; i < DISPLAY_FRAGMENT_COUNT; ++i) {
        unsigned offset = i * DISPLAY_FRAGMENT_SIZE;
        if (!(stream->known & (1U << i)) ||
            memcmp(stream->sent + offset, stream->target + offset, DISPLAY_FRAGMENT_SIZE))
            stream->dirty |= 1U << i;
    }
}

/* Replace obsolete waiting content with the latest complete, space-padded screen. */
void display_stream_submit(DisplayStream *stream, const uint8_t *text) {
    memcpy(stream->target, text, sizeof(stream->target));
    stream->valid = true;
    update_dirty(stream);
}

/* Offer the next changed fragment fairly, including newer content after a failed send. */
bool display_stream_peek(DisplayStream *stream, uint8_t *fragment, uint8_t text[3]) {
    stream->offering = false;
    if (!stream->valid)
        return false;
    for (unsigned n = 0; n < DISPLAY_FRAGMENT_COUNT; ++n) {
        unsigned i = (stream->cursor + n) % DISPLAY_FRAGMENT_COUNT;
        if (!(stream->dirty & (1U << i)))
            continue;
        stream->fragment = i;
        memcpy(stream->offered, stream->target + i * DISPLAY_FRAGMENT_SIZE, DISPLAY_FRAGMENT_SIZE);
        memcpy(text, stream->offered, DISPLAY_FRAGMENT_SIZE);
        *fragment = i;
        stream->offering = true;
        return true;
    }
    return false;
}

/* Remember exactly the accepted fragment; a newer target remains pending if it differs. */
void display_stream_accept(DisplayStream *stream) {
    if (!stream->offering)
        return;
    unsigned i = stream->fragment;
    memcpy(stream->sent + i * DISPLAY_FRAGMENT_SIZE, stream->offered, DISPLAY_FRAGMENT_SIZE);
    stream->known |= 1U << i;
    stream->forced &= (uint8_t)~(1U << i);
    stream->cursor = (i + 1) % DISPLAY_FRAGMENT_COUNT;
    stream->offering = false;
    update_dirty(stream);
}

/* Forget previous output when BACCAble relinquishes the display. */
void display_stream_reset(DisplayStream *stream) { memset(stream, 0, sizeof(*stream)); }

/* Restore or maintain nonblank text without repeatedly restarting a pending restoration. */
void display_stream_refresh(DisplayStream *stream) {
    if (!stream->valid || stream->forced)
        return;
    for (unsigned i = 0; i < sizeof(stream->target); ++i) {
        if (stream->target[i] != ' ') {
            stream->forced = (1U << DISPLAY_FRAGMENT_COUNT) - 1U;
            update_dirty(stream);
            return;
        }
    }
}
