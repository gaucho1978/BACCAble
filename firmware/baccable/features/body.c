#include "features/body.h"
#include "features/display_stream.h"
#include "features/parking_mirrors.h"
#include "storage/flash_records.h"

/* Preserve the established IPC pacing while avoiding redundant text fragments. */
#define DISPLAY_FRAGMENT_INTERVAL_MS 50U
#define DISPLAY_KEEPALIVE_INTERVAL_MS 500U

#if defined(BACCABLE_BH)

static DisplayStream screen;

/* Accept new dashboard content when BACCAble display output is allowed. */
void body_display_submit(const uint8_t *text) {
    if (!chassis_state.stability_inverted)
        display_stream_submit(&screen, text);
}

/* Restore BACCAble content after factory display activity when allowed. */
void body_display_refresh(void) {
    if (!chassis_state.stability_inverted)
        display_stream_refresh(&screen);
}

/* Prepare body-bus features and restore saved mirror positions. */
void body_init() {
    // let's open the can bus because we may need data
    can_set_bitrate(CAN_BITRATE_125K); // set can speed to 125kpbs
    can_enable();                      // enable can port

    // prepare msg to send:
    // total frame number is on byte 0 from bit 7 to 3
    display_state.telematic_display_info_msg_data[0] =
        (display_state.telematic_display_info_msg_data[0] & ~0xF8) |
        ((display_state.telematic_display_info_field_total_frame_number << 3) & 0xF8);
    // infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in
    // progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...)
    display_state.telematic_display_info_msg_data[1] =
        (display_state.telematic_display_info_msg_data[1] & ~0x3F) |
        ((display_state.telematic_display_info_field_info_code) & 0x3F);
    // Single-byte IPC glyphs occupy the low bytes; the high bytes remain zero.
    display_state.telematic_display_info_msg_data[2] = 0;
    display_state.telematic_display_info_msg_data[4] = 0;
    display_state.telematic_display_info_msg_data[6] = 0;

    // load stored params
    mirrors_state.left_park_mirror_horizontal_pos = (uint8_t)mirror_positions_read(1);
    mirrors_state.left_park_mirror_vertical_pos = (uint8_t)mirror_positions_read(2);
    mirrors_state.right_park_mirror_horizontal_pos = (uint8_t)mirror_positions_read(3);
    mirrors_state.right_park_mirror_vertical_pos = (uint8_t)mirror_positions_read(4);
    mirrors_state.park_mirror_operative_position_not_stored = (uint8_t)mirror_positions_read(5);
    mirrors_state.left_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(6);
    mirrors_state.left_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(7);
    mirrors_state.right_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(8);
    mirrors_state.right_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(9);
}

/* Update dashboard content, mirrors and enabled body-bus functions. */
void body_process() {
    if (chassis_state.stability_inverted) {
        display_stream_reset(&screen);
    } else if (currentTime - display_state.last_sent_telematic_display_info_msg_time >=
               DISPLAY_FRAGMENT_INTERVAL_MS) {
        /* Suppressing identical submissions must not let an otherwise idle display expire. */
        if (currentTime - display_state.last_sent_telematic_display_info_msg_time >=
            DISPLAY_KEEPALIVE_INTERVAL_MS)
            display_stream_refresh(&screen);
        uint8_t fragment, text[3];
        if (display_stream_peek(&screen, &fragment, text)) {
            uint8_t *data = display_state.telematic_display_info_msg_data;
            data[0] = (data[0] & ~0x07) | ((fragment >> 2) & 0x07);
            data[1] = (data[1] & ~0xC0) | ((fragment << 6) & 0xC0);
            data[3] = text[0];
            data[5] = text[1];
            data[7] = text[2];
            if (can_tx(&display_state.telematic_display_info_msg_header, data) == HAL_OK) {
                display_stream_accept(&screen);
                display_state.last_sent_telematic_display_info_msg_time = currentTime;
            }
        }
    }

    parking_mirrors_process();
}

/* Remember the mirror positions needed for parking and normal driving. */
uint8_t mirror_positions_save(void) {
    uint16_t values[9] = {mirrors_state.left_park_mirror_horizontal_pos,
                          mirrors_state.left_park_mirror_vertical_pos,
                          mirrors_state.right_park_mirror_horizontal_pos,
                          mirrors_state.right_park_mirror_vertical_pos,
                          mirrors_state.park_mirror_operative_position_not_stored,
                          mirrors_state.left_mirror_horizontal_operative_pos,
                          mirrors_state.left_mirror_vertical_operative_pos,
                          mirrors_state.right_mirror_horizontal_operative_pos,
                          mirrors_state.right_mirror_vertical_operative_pos};
    return flash_record_save(SETTINGS_RECORD, 0x201, values, sizeof(values)) ? 0 : 255;
}

/* Restore a saved mirror position, or report that it is unavailable. */
uint16_t mirror_positions_read(uint8_t id) {
    if (id < 1 || id > 9)
        return 0;
    uint16_t values[9] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
    flash_record_load(SETTINGS_RECORD, 0x201, values, sizeof(values));
    return values[id - 1];
}

#endif
