#include "vehicle/standard_frames.h"
#include "app/powertrain.h"
#include "features/body.h"
/* CAN ID 0x00000090. */
/* Restore BACCAble text when factory display content would replace it. */
void vehicle_handle_display_content(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_BH)
    /* Restore after factory text without restarting a partially transmitted view. */
    body_display_refresh();
#endif
    // on BH can bus, slow bus at 125kbps, this message contains:

    // total frame number is on byte 0 from bit 7 to 3
    // frame number is on byte 0 from bit 2 to 0 and byte1 from bit7 to 6
    // infoCode is on byte1 from bit 5 to 0 (0x12=phone connected, 0x13=phone disconnected, 0x15=call in
    // progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...) UTF text 1 is on byte 2 and
    // byte 3 UTF text 2 is on byte 4 and byte 5 UTF text 3 is on byte 6 and byte 7
}

/* CAN ID 0x0000025A. */
/* Apply the configured driving-mode presentation on the dashboard. */
void vehicle_handle_drive_style_display(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 1)
        return;
    // msg received by DriveStyle control module
#if defined(BACCABLE_BH)
    if (chassis_state.stability_inverted) {
        if (telemetry_state.drive_mode != 0x0C) { // if we are not in race
            if ((frame_data[0] & 0x7C) !=
                0x04) { // drive style is on byte 0 from bit 2 to 6 (without bit shift is: 0x40=natural,
                        // 0x20=dynamic, 0x10=Allweather, 0x04=race)
                frame_data[0] = (frame_data[0] & ~0x7C) | 0x04;
                can_forward(rx_header, frame_data); // transmit the modified packet
            }
        }
    }
#endif
}

/* CAN ID 0x00000356. */
/* Apply the preference for suppressing a blinking odometer. */
void vehicle_handle_odometer(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 5)
        return;

#if defined(BACCABLE_BH)
    if (display_state.disable_odometer_blink) {
        if ((frame_data[4] & 0x04) == 0x04) {   // if proxy align is required (SysEOLsts=1)
            frame_data[4] &= (uint8_t)~0x04;    // set to zero the SysEOLsts
            can_forward(rx_header, frame_data); // retransmit the packet
        }
    }
#endif
}

/* CAN ID 0x0000046C. */
/* Update body-side driving-mode presentation and related requests. */
void vehicle_handle_body_drive_mode(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 8)
        return;

#if defined(BACCABLE_BH)

    if (telemetry_state.drive_mode !=
        (frame_data[7] & 0x1F)) { // RDNA mode was changed, reset the ESCandTCinversion
        chassis_state.stability_inverted = 0;
    }
    // current DNA mode, also called "Drive Style Status" (RDNA mode) is on byte 7, from bit 0 to bit 4
    // (0x0=Natural, 0x2=dynamic, 0x4=AllWeather, 0xC=race)
    telemetry_state.drive_mode = frame_data[7] & 0x1F; // 1F is the mask from bit 4 to 0
    if (chassis_state.stability_inverted) {
        if (telemetry_state.drive_mode != 0x0C) { // if not in race
            frame_data[7] =
                (frame_data[7] & ~0x1F) |
                0x0C; // set Race mode (0x30) to show on IPC the race screen (msg from body to IPC, ETM, ESEM)
            can_forward(rx_header, frame_data); // transmit the modified packet

            // problem as expected status_led_activity();
        }
    }

    // turn indicators are on byte 6, bit 2 and 1
    mirrors_state.turn_indicator = (frame_data[6] >> 1) & 0x03; // 0= center, 1=right, 2=left
#endif
}

/* CAN ID 0x000004AF. */
/* Track stability-control status and the permitted dashboard presentation. */
void vehicle_handle_stability_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 2)
        return;

#if defined(BACCABLE_C1) ||                                                                                  \
    defined(BACCABLE_C2) // we will test it to understand messages changing style on IPC
    if (chassis_state.stability_inverted) {
        if (telemetry_state.drive_mode != 0x30) { // if we're not in race
            if (((frame_data[0] & 0x01) != 0x01) ||
                ((frame_data[1] & 0x04) != 0x04)) { // if not set as expected in race
                frame_data[0] = frame_data[0] |
                                0x01; // set functionstatus bit (on C1 it is a msg from BSM to TCM, ORc, IPC,
                                      // ECM, DTCM, DCTM, DASM, BCM) (on C2 it is a msg from BSM to HALF, BCM)
                frame_data[1] =
                    frame_data[1] |
                    0x04; // set function2status bit (on C1 it is a msg from BSM to IPC, DASM, CDCM)
                can_forward(rx_header, frame_data); // transmit the modified packet
            }
        }
    }
#endif
}

/* CAN ID 0x00000545. */
/* Apply requested dashboard-brightness feedback. */
void vehicle_handle_brightness(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_C1)
    if (rx_header->DLC == 8) {
        memcpy(&dashboard_state.dashboard_blink_msg_data, frame_data, 8);
    }
#endif
    // only if  lights are ON, and therefore the dashboard is  set to max brightness: setting byte 5 to 0x00,
    // the brightness increases for around 100msec (this works for any value between 0x and 7x ) only if
    // lights are OFF, and therefore the dashboard is set to min brightness: setting byte 5 to 0xF0, the
    // brightness reduces for around 100msec (this works for any value between Dx and Fx) this is the test
    // message to increase brightness: 0x88 0x20 0xC3 0x24 0x00 0x14 0x30 0x00
}

/* CAN ID 0x000005AC. */
/* Apply pending audible-feedback requests to the vehicle's chime report. */
void vehicle_handle_chime(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_BH)
    if (display_state.request_to_play_chime == 1) { // if there is a request to play sound
        display_state.request_to_play_chime = 0;
        // copy message
        memcpy(display_state.chime_msg_data, frame_data, rx_header->DLC);
        // enable chime, by changing message
        display_state.chime_msg_header.DLC = rx_header->DLC;
        display_state.chime_msg_data[0] =
            (display_state.chime_msg_data[0] & 0b00111111); // set bit 7 and 6 to zero (chime type 0)
        display_state.chime_msg_data[1] = (display_state.chime_msg_data[1] & 0b00111111) |
                                          0b01000000; // byte1 bit 7 and 6 = 01 (seatbelt alarm active)
        display_state.chime_msg_data[3] = display_state.chime_msg_data[3] | 0xE0; // max volume
        can_tx(&display_state.chime_msg_header, display_state.chime_msg_data);    // send msg
        status_led_activity();
    }
#endif
}
