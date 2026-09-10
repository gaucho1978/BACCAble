#include "state/security.h"
SecurityState security_state = {
    .reserved = 0,
#if defined(BACCABLE_C1) || defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER)
    .immobilizer_enabled = 1,
    .panic_alarm_activated = 0,
    .panic_alarm_start_msg_header =
        {
            {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x18DAC7F1, .DLC = 3},
            {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = 0x1E340041, .DLC = 4},
            {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x1EF, .DLC = 8},
            {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x1EF, .DLC = 8},
        },
    .panic_alarm_start_msg_data =
        {
            {
                0x02,
                0x11,
                0x01,
                0x00,
            },
            {
                0x88,
                0x20,
                0x15,
                0x00,
            },
            {0x42, 0x02, 0xE2, 0x00, 0x00, 0x00, 0x01, 0x56},
            {0x00, 0x00, 0xE2, 0x00, 0x00, 0x00, 0x00, 0x00},
        },
    .flood_the_bus = 0,
    .flood_the_bus_start_time = 0,
    .flood_the_bus_last_time_sent = 0,
#endif
};
