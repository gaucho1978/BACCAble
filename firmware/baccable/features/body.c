#include "features/body.h"
#include "storage/flash_records.h"

#if defined(BACCABLE_BH)

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

void body_process() {
    if (display_state.request_to_send_one_frame >
        0) { // if requested by a message received from master baccable
        // send one msg to write something on the dashboard each 50msec (one frame each 300msec)
        if (currentTime - display_state.last_sent_telematic_display_info_msg_time > 50) {
            display_state.last_sent_telematic_display_info_msg_time = currentTime;
            // prepare msg to send:
            // frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
            display_state.telematic_display_info_msg_data[0] =
                (display_state.telematic_display_info_msg_data[0] & ~0x07) |
                ((display_state.telematic_display_info_field_frame_number >> 2) & 0x07);
            display_state.telematic_display_info_msg_data[1] =
                (display_state.telematic_display_info_msg_data[1] & ~0xC0) |
                ((display_state.telematic_display_info_field_frame_number << 6) & 0xC0);

            // UTF text 1 is on byte 2 and byte 3
            display_state.telematic_display_info_msg_data[3] =
                dashboard_state.dashboard_page_string_array[display_state.params_string_char_index];
            display_state.params_string_char_index++; // prepare to send next char
            // UTF text 2 is on byte 4 (set to zero ) and byte 5
            display_state.telematic_display_info_msg_data[5] =
                dashboard_state.dashboard_page_string_array[display_state.params_string_char_index];
            display_state.params_string_char_index++; // prepare to send next char
            // UTF text 3 is on byte 6 (set to zero) and byte 7
            display_state.telematic_display_info_msg_data[7] =
                dashboard_state.dashboard_page_string_array[display_state.params_string_char_index];
            display_state.params_string_char_index++; // prepare to send next char
            // send it
            can_tx(&display_state.telematic_display_info_msg_header,
                   display_state.telematic_display_info_msg_data); // transmit the packet

            display_state.telematic_display_info_field_frame_number++; // prepare for next frame to send
            if (display_state.params_string_char_index >=
                DASHBOARD_MESSAGE_MAX_LENGTH) {             // if we sent the entire string
                display_state.params_string_char_index = 0; // prepare to send first char of the string
                display_state.telematic_display_info_field_frame_number = 0; // prepare to send first frame
                display_state.request_to_send_one_frame -= 1;
                status_led_activity();
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

uint16_t mirror_positions_read(uint8_t id) {
    if (id < 1 || id > 9)
        return 0;
    uint16_t values[9] = {0, 0, 0, 0, 1, 0, 0, 0, 0};
    flash_record_load(SETTINGS_RECORD, 0x201, values, sizeof(values));
    return values[id - 1];
}

#endif
