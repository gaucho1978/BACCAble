#include "vehicle/standard_frames.h"
#include "vehicle/frame_handlers.h"

/* Route supported vehicle updates to their features and honor explicit message-routing requests. */
void vehicle_dispatch_standard(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
#if defined(BACCABLE_C1)
    if (settings_state.route_msg_enabled == 1) {
        if (diagnostics_state.route_std_id_msg == 1) { // if we have to do it (std msg id route request

            if (rx_header->StdId == diagnostics_state.route_msg_id) { // received msg to route
                diagnostics_state.route_std_id_msg =
                    0xFF; // set this to disable the request. only one message is routed to avoid bus flood
                if (diagnostics_state.route_offset < rx_header->DLC) { // send only if offset is correct
                    uint8_t sizeToCopy = 5;
                    if ((rx_header->DLC - diagnostics_state.route_offset) < sizeToCopy)
                        sizeToCopy = rx_header->DLC - diagnostics_state.route_offset;
                    memcpy(&diagnostics_state.route_msg_data[3], &frame_data[diagnostics_state.route_offset],
                           sizeToCopy);
                    if (sizeToCopy < 5)
                        memset(&diagnostics_state.route_msg_data[3 + sizeToCopy], 0x00, 5 - sizeToCopy);

                    can_tx(&diagnostics_state.route_msg_header, diagnostics_state.route_msg_data);
                    status_led_activity();
                }
            }
        }
    }
#endif

    switch (rx_header->StdId) { // messages in this switch is on C can bus, only when on different bus, the
                                // comments explicitly tells if it is on another can bus

    case 0x00000090:
        vehicle_handle_display_content(rx_header, frame_data);
        break;
    case 0x000000FB:
        vehicle_handle_engine_torque(rx_header, frame_data);
        break;
    case 0x000000FC:
        vehicle_handle_engine_status(rx_header, frame_data);
        break;
    case 0x00000101:
        vehicle_handle_vehicle_speed(rx_header, frame_data);
        break;
    case 0x00000192:
        vehicle_handle_gear_lever(rx_header, frame_data);
        break;
    case 0x000001EF:
        vehicle_handle_body_commands(rx_header, frame_data);
        break;
    case 0x00000226:
        vehicle_handle_start_stop_status(rx_header, frame_data);
        break;
    case 0x0000025A:
        vehicle_handle_drive_style_display(rx_header, frame_data);
        break;
    case 0x000002ED: // message to dashboard containing shift indicator
        vehicle_handle_shift_indicator(rx_header, frame_data);
        break;
    case 0x000002EF:
        vehicle_handle_gear(rx_header, frame_data);
        break;
    case 0x000002FA:
        vehicle_handle_steering_controls(rx_header, frame_data);
        break;
    case 0x00000354:
        vehicle_handle_body_lights(rx_header, frame_data);
        break;
    case 0x00000356:
        vehicle_handle_odometer(rx_header, frame_data);
        break;
    case 0x00000384:
        vehicle_handle_drive_mode(rx_header, frame_data);
        break;
    case 0x000003E8:
        vehicle_handle_body_gear(rx_header, frame_data);
        break;
    case 0x00000412:
        vehicle_handle_accelerator(rx_header, frame_data);
        break;
    case 0x0000041A:
        vehicle_handle_battery(rx_header, frame_data);
        break;
    case 0x0000046C:
        vehicle_handle_body_drive_mode(rx_header, frame_data);
        break;
    case 0x000004AF:
        vehicle_handle_stability_status(rx_header, frame_data);
        break;
    case 0x000004B1:
        vehicle_handle_start_stop(rx_header, frame_data);
        break;
    case 0x000004B2:
        vehicle_handle_oil(rx_header, frame_data);
        break;
    case 0x00000545:
        vehicle_handle_brightness(rx_header, frame_data);
        break;
    case 0x000005A0:
        vehicle_handle_lane_button(rx_header, frame_data);
        break;
    case 0x000005A5:
        vehicle_handle_cruise_control(rx_header, frame_data);
        break;
    case 0x000005A6:
        vehicle_handle_mirror_position(rx_header, frame_data);
        break;
    case 0x000005A8:
        vehicle_handle_transmission_mode(rx_header, frame_data);
        break;
    case 0x000005AC:
        vehicle_handle_chime(rx_header, frame_data);
        break;
    case 0x000005AE:
        vehicle_handle_regeneration(rx_header, frame_data);
        break;
    case 0x000005B0:
        vehicle_handle_park_assist(rx_header, frame_data);
        break;
    case 0x0000073C:
        vehicle_handle_adaptive_cruise(rx_header, frame_data);
        break;
    case 0x0000073E:
        vehicle_handle_light_animation(rx_header, frame_data);
        break;
    default:
    }
}
