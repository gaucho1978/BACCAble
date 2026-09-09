#include "vehicle/standard_frames.h"
#include "app/powertrain.h"
/* CAN ID 0x00000354. */
void vehicle_handle_body_lights(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {

#if defined(BACCABLE_BH)
    /*
     msg from body to IPC and others:
            0x354 : 0x10 0x00 0x00 0x00 (fendinebbia frontali)
            0x354 : 0x04 0x00 0x00 0x00 (abbaglianti)
            0x354 : 0x02 0x00 0x00 0x00 (luci di posizione sinistra)
            0x354 : 0x00 0x80 0x00 0x00 (freccia sinistra)
            0x354 : 0x00 0x40 0x00 0x00 (anabbaglianti)
            0x354 : 0x00 0x04 0x00 0x00 (fendinebbia posteriori)
            0x354 : 0x00 0x00 0x80 0x00 (luci di posizione destra)
            0x354 : 0x00 0x00 0x20 0x00 (freccia destra)
            0x354 : 0x00 0x00 0x08 0x00 (automatic high beam)
            0x354 : 0x00 0x00 0x04 0x00 (stop)
            0x354 : 0x00 0x00 0x02 0x00 (attivazione frecce sinistra)
            0x354 : 0x00 0x00 0x01 0x00 (attivazione frecce destra)
            0x354 : 0x00 0x00 0x00 0x40 (automatic low beam)
     */

#endif
}

/* CAN ID 0x0000073E. */
void vehicle_handle_light_animation(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 4)
        return;

    /*
            Message 0x73E is a message on C2 bus from Body to adaptive front light module
            It flows on C1 bus too, directed elsewhere
            Period: 250msec
                    -0x73E : 0x10 0x00 0x00 0x00 (fendinebbia frontali solo su C2)
                    -0x73E : 0x04 0x00 0x00 0x00 (abbaglianti)
                    -0x73E : 0x02 0x00 0x00 0x00 (luce di parcheggio sinistra solo su C1)
                    -0x73E : 0x00 0x80 0x00 0x00 (freccia sinistra solo su C1)
                    -0x73E : 0x00 0x40 0x00 0x00 (anabbaglianti solo su C2)
                    -0x73E : 0x00 0x00 0x80 0x00 (luce di parcheggio destra solo su C1)
                    -0x73E : 0x00 0x00 0x20 0x00 (freccia destra solo su C1)
                    -0x73E : 0x00 0x00 0x08 0x00 (automatic high beam, solo su C2)
                    -0x73E : 0x00 0x00 0x04 0x00 (stop solo su C1)
                    -0x73E : 0x00 0x00 0x02 0x00 (attivazione frecce sinistra, solo su C2)
                    -0x73E : 0x00 0x00 0x01 0x00 (attivazione frecce destra, solo su C2)

     */

#if defined(BACCABLE_C1)

    if (settings_state.lights_animation_enabled) {
        if (comfort_state.lights_animation_state_machine) {
            switch (comfort_state.lights_animation_state_machine) {
            case 22: // High beam light (C1 and C2) 	0x04 0x00 0x00 0x00
            case 21:
                frame_data[0] = 0x04;
                frame_data[1] = 0x00;
                frame_data[2] = 0x00;
                frame_data[3] = 0x00;
                break;
            case 20: // front fog light(only on C2)	0x10 0x00 0x00 0x00
            case 19:
                frame_data[0] = 0x10;
                frame_data[1] = 0x00;
                frame_data[2] = 0x00;
                frame_data[3] = 0x00;
                break;
            case 18: // left park light (C1)			0x02 0x00 0x00 0x00
            case 17:
                frame_data[0] = 0x02;
                frame_data[1] = 0x00;
                frame_data[2] = 0x00;
                frame_data[3] = 0x00;
                break;
            case 16: // left direction light (C1)			0x00 0x80 0x00 0x00
                     // left direction light activation(C2)	0x00 0x00 0x02 0x00
            case 15:
                frame_data[0] = 0x00;
                frame_data[1] = 0x80;
                frame_data[2] = 0x02;
                frame_data[3] = 0x00;
                break;
            case 14: // low beam light (C2)			0x00 0x40 0x00 0x00
            case 13:
                frame_data[0] = 0x00;
                frame_data[1] = 0x40;
                frame_data[2] = 0x00;
                frame_data[3] = 0x00;
                break;
            case 12: // right direction light (C1)			0x00 0x00 0x20 0x00
                     // right direction light activation(C2)	0x00 0x00 0x01 0x00
            case 11:
                frame_data[0] = 0x00;
                frame_data[1] = 0x00;
                frame_data[2] = 0x21;
                frame_data[3] = 0x00;
                break;
            case 10: // right parking light (C1)		0x00 0x00 0x80 0x00
            case 9:
                frame_data[0] = 0x00;
                frame_data[1] = 0x00;
                frame_data[2] = 0x80;
                frame_data[3] = 0x00;
                break;
            case 8: // stop light(C1)				0x00 0x00 0x04 0x00
            case 7:
                frame_data[0] = 0x00;
                frame_data[1] = 0x00;
                frame_data[2] = 0x04;
                frame_data[3] = 0x00;
                break;
            case 6:
            case 5:
                comfort_state.lights_animation_state_machine = 1; // stop the animation
                break;
            case 4: // not used
            case 3: // not used
                break;
            case 2: // not used
            case 1: // not used
                break;
            default: // do nothing
                break;
            }

            // send message
            can_forward(rx_header, frame_data); // retransmit the packet
            comfort_state.lights_animation_state_machine--;
        }
    }
#endif
}
