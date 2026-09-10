#include "features/body.h"
#include "features/display_stream.h"
#include "storage/flash_records.h"

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
    // I don't use UTF chars, but ascii, so bytes 2,4,6 can be set to zero
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
    } else if (currentTime - display_state.last_sent_telematic_display_info_msg_time >= 50) {
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

    if (settings_state.park_mirror) { // if function parkmirror is enabled

        if (telemetry_state.current_gear == 0x0E) { // if reverse gear is selected
            if (!mirrors_state
                     .restore_operative_mirrors_position) { // if we are not returning to operative position
                switch (mirrors_state.turn_indicator) {
                case 0x02: // left arrow inserted
                    if (!mirrors_state.left_park_mirror_position_required &&
                        !mirrors_state.right_park_mirror_position_required)
                        mirrors_state.store_operative_mirror_position =
                            1; // store current mirror position, if mirror was not previously lowered
                    mirrors_state.left_park_mirror_position_required =
                        1; // Enable sending command to move mirror
                    break;
                case 0x01: // right arrow inserted
                    if (!mirrors_state.left_park_mirror_position_required &&
                        !mirrors_state.right_park_mirror_position_required)
                        mirrors_state.store_operative_mirror_position =
                            1; // store current mirror position, if mirror was not previously lowered
                    mirrors_state.right_park_mirror_position_required =
                        1; // Enable sending command to move mirror
                    break;
                default:
                }
            }
        } else {
            if (mirrors_state.left_park_mirror_position_required ||
                mirrors_state.right_park_mirror_position_required) { // if mirrors are potentially not in
                                                                     // operative position,
                mirrors_state.restore_operative_mirrors_position =
                    1; // request to restore mirrors to their original position
                mirrors_state.restore_operative_mirrors_position_request_time = currentTime;
            }
            mirrors_state.left_park_mirror_position_required =
                0; // stop sending message to set Park position for mirrors
            mirrors_state.right_park_mirror_position_required =
                0; // stop sending message to set Park position for mirrors
        }

        // Prepare msg to send: set Operative position of the mirrors
        mirrors_state.park_mirror_msg_data[0] = mirrors_state.left_mirror_horizontal_operative_pos;
        mirrors_state.park_mirror_msg_data[1] = mirrors_state.left_mirror_vertical_operative_pos;
        mirrors_state.park_mirror_msg_data[2] = mirrors_state.right_mirror_horizontal_operative_pos;
        mirrors_state.park_mirror_msg_data[3] = mirrors_state.right_mirror_vertical_operative_pos;

        // Prepare msg to send: if required, set park position of the mirrors
        if (mirrors_state.left_park_mirror_position_required) {
            mirrors_state.park_mirror_msg_data[0] = mirrors_state.left_park_mirror_horizontal_pos;
            mirrors_state.park_mirror_msg_data[1] = mirrors_state.left_park_mirror_vertical_pos;
        }
        if (mirrors_state.right_park_mirror_position_required) {
            mirrors_state.park_mirror_msg_data[2] = mirrors_state.right_park_mirror_horizontal_pos;
            mirrors_state.park_mirror_msg_data[3] = mirrors_state.right_park_mirror_vertical_pos;
        }

        if (mirrors_state.left_park_mirror_position_required ||
            mirrors_state.right_park_mirror_position_required ||
            mirrors_state.restore_operative_mirrors_position) {   // if required
            if (!mirrors_state.store_operative_mirror_position) { // if operative position was stored
                if (currentTime - mirrors_state.last_park_mirror_msg_time >
                    900) { // each 1000msec send a packet
                    can_tx(&mirrors_state.park_mirror_msg_header,
                           mirrors_state.park_mirror_msg_data); // send msg
                    mirrors_state.last_park_mirror_msg_time = currentTime;
                }
            }
            if (mirrors_state.restore_operative_mirrors_position) {
                if (currentTime - mirrors_state.restore_operative_mirrors_position_request_time >
                    15000) { // after 15 seconds
                    mirrors_state.restore_operative_mirrors_position = 0;
                }
            }
        }
    }
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
