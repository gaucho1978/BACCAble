#include "vehicle/start_stop_status.h"

void vehicle_handle_start_stop_status(const CAN_RxHeaderTypeDef *rx_header, uint8_t *frame_data) {
    if (rx_header->DLC < 3)
        return;
#if defined(BACCABLE_C1)
    if (rx_header->DLC >= 3) {
        // fill a variable with start&stop Status (byte2 bit 3 and 2 = 1 means S&S disabled)
        if (((frame_data[2] >> 2) & 0x03) == 0x01) {
            comfort_state.start_andstop_car_status = 0; // S&S disabled in car

        } else {
            comfort_state.start_andstop_car_status = 1; // S&S enabled in car (default in giulias)
        }
    }
#endif
}
