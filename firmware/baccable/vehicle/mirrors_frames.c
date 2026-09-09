#include "vehicle/standard_frames.h"
#include "app/powertrain.h"
/* CAN ID 0x000005A6. */
void vehicle_handle_mirror_position(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 5)
        return;

// leftHorizontal,leftVertical, rightHorizontal,rightVertical is from byte x (one byte each one)
#if defined(BACCABLE_BH)
    mirrors_state.park_mirrors_steady = ((frame_data[3] & 0x40) == 0x00) &&
                                        ((frame_data[3] & 0x10) == 0x00); // 1 if side mirrors are not moving
    if (mirrors_state.store_operative_mirror_position) { // if it was requested to store Operative side
                                                         // mirrors position
        if (mirrors_state.park_mirrors_steady) {         // if mirror movement is not in progress
            mirrors_state.store_operative_mirror_position = 0;
            if (mirrors_state.park_mirror_operative_position_not_stored) { // if position was not previously
                                                                           // stored, read it and store it
                mirrors_state.park_mirror_operative_position_not_stored = 0;
                mirrors_state.left_mirror_horizontal_operative_pos = frame_data[0];
                mirrors_state.left_mirror_vertical_operative_pos = frame_data[1];
                mirrors_state.right_mirror_horizontal_operative_pos = frame_data[2];
                mirrors_state.right_mirror_vertical_operative_pos =
                    ((frame_data[3] & 0x0f) << 4) | (frame_data[4] >> 4);
                mirror_positions_save();
            } else { // otherwise get it from memory
                mirrors_state.left_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(6);
                mirrors_state.left_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(7);
                mirrors_state.right_mirror_horizontal_operative_pos = (uint8_t)mirror_positions_read(8);
                mirrors_state.right_mirror_vertical_operative_pos = (uint8_t)mirror_positions_read(9);
            }
        }
    }
    if (mirrors_state.store_current_park_mirror_position) {
        if (mirrors_state.park_mirrors_steady) { // if mirror movement is not in progress
            mirrors_state.store_current_park_mirror_position = 0;
            mirrors_state.left_park_mirror_horizontal_pos = frame_data[0];
            mirrors_state.left_park_mirror_vertical_pos = frame_data[1];
            mirrors_state.right_park_mirror_horizontal_pos = frame_data[2];
            mirrors_state.right_park_mirror_vertical_pos =
                ((frame_data[3] & 0x0f) << 4) | (frame_data[4] >> 4);
            mirror_positions_save(); // save it permanently on BH!
        }
    }

    if (mirrors_state
            .restore_operative_mirrors_position) { // if we are returning to original operative position
        // if we completed the return to the original operative position of the mirrors, set the variable as
        // completed
        if (((frame_data[3] >> 4) & 0b00000101) == 0x00) { // if mirrors are steady
            if ((frame_data[0] >= mirrors_state.left_mirror_horizontal_operative_pos - 1) &&
                (frame_data[0] <= mirrors_state.left_mirror_horizontal_operative_pos + 1)) {
                if ((frame_data[1] >= mirrors_state.left_mirror_vertical_operative_pos - 1) &&
                    (frame_data[1] <= mirrors_state.left_mirror_vertical_operative_pos + 1)) {
                    if ((frame_data[2] >= mirrors_state.right_mirror_horizontal_operative_pos - 1) &&
                        (frame_data[2] <= mirrors_state.right_mirror_horizontal_operative_pos + 1)) {
                        if (((((frame_data[3] & 0x0f) << 4) | (frame_data[4] >> 4)) >=
                             mirrors_state.right_mirror_vertical_operative_pos - 1) &&
                            ((((frame_data[3] & 0x0f) << 4) | (frame_data[4] >> 4)) <=
                             mirrors_state.right_mirror_vertical_operative_pos + 1)) {
                            mirrors_state.restore_operative_mirrors_position =
                                0; // save the fact that the requested operation was successully completed
                        }
                    }
                }
            }
        }
    }
#endif
}

/* CAN ID 0x000005A8. */
void vehicle_handle_transmission_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 8)
        return;

// when in race, byte 4 bit 3-6 has value 6
#if defined(BACCABLE_C1)
    if (chassis_state.stability_inverted) {
        if ((frame_data[4] & 0x78) !=
            0x30) { // if not race (on C1 it is a msg from TCM or DCTM and received by ECM)
            frame_data[4] = (frame_data[4] & ~0x78) | 0x30; // set track mode (race)
            uint8_t tmpCounter = (frame_data[6] & 0x0F) + 1;
            if (tmpCounter > 0x0F)
                tmpCounter = 0;
            frame_data[6] = (frame_data[6] & 0xF0) | tmpCounter;        // increment counter
            frame_data[7] = frame_checksum(frame_data, rx_header->DLC); // update CRC
            can_forward(rx_header, frame_data);                         // transmit the modified packet
            // can_process(); //we try to send it ASAP - Commented since it is not solving the delay problem
            // as expected
        }
    }
#endif

#if defined(BACCABLE_BH)
    if (settings_state.park_mirror) {
        // BH movement of mirror is requested
        if (mirrors_state.left_park_mirror_position_required ||
            mirrors_state.right_park_mirror_position_required ||
            mirrors_state.restore_operative_mirrors_position) { // if required
            if (!mirrors_state.store_operative_mirror_position &&
                !mirrors_state
                     .park_mirrors_steady) { // if Operative position was stored and mirror is not steady
                can_tx(&mirrors_state.park_mirror_msg_header, mirrors_state.park_mirror_msg_data); // send msg
            }
        }
    }
#endif
}
