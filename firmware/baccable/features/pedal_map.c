#include "app/powertrain.h"
#if defined(BACCABLE_C1)
typedef struct {
    uint8_t command;
    char reply;
    uint8_t amplitude;
    float increase;
    float decrease;
} PedalMap;
static const PedalMap maps[] = {{0x00, 'B', 0, 0, 0},
                                {0x49, 'A', 96, 1.6f, 0.6f},
                                {0x92, 'N', 128, 3.2f, 1.6f},
                                {0xdb, 'D', 192, 2.0f, 3.2f},
                                {0x24, 'R', 230, 2.4f, 1.8f}};

/* Request a new accelerator-response map from the pedal controller. */
void pedal_booster_set_map(uint8_t selection) {
    if (selection == 7)
        selection = telemetry_state.drive_mode == 0x30 ? 6 : 4;
    if (selection == 8)
        selection = 3;
    unsigned index = selection >= 2 && selection <= 6 ? selection - 2 : 0;
    const PedalMap *map = &maps[index];
    uint8_t message[UART1_BUFFER_SIZE] = {'#', 0xb6, map->command};
    if (index) {
        int power = settings_state.pedal_booster_enabled == 8 ? -10 : settings_state.pedal_map_power;
        if (power < -10)
            power = -10;
        if (power > 10)
            power = 10;
        int delta = (int)(power * (power >= 0 ? map->increase : map->decrease) + (power >= 0 ? 0.5f : -0.5f));
        message[3] = map->amplitude + delta;
        message[4] = 128;
        message[5] = 154;
    }
    message[8] = frame_checksum(message, sizeof(message));
    pedal_uart_send(message, sizeof(message));
}

/* Check whether the selected pedal behavior needs a controller update. */
uint8_t pedal_booster_needs_update(void) {
    unsigned selection = settings_state.pedal_booster_enabled;
    if (selection == 7)
        selection = telemetry_state.drive_mode == 0x30 ? 6 : 4;
    if (selection == 8)
        selection = 3;
    if (selection == 0)
        return 0;
    if (selection == 1) {
        switch (telemetry_state.drive_mode) {
        case 0x00:
            selection = 4;
            break;
        case 0x08:
            selection = 5;
            break;
        case 0x10:
            selection = 3;
            break;
        case 0x30:
            selection = 6;
            break;
        default:
            selection = 2;
            break;
        }
    }
    return selection < 2 || selection > 6 ||
           pedal_state.current_schizzaforte_map != maps[selection - 2].reply;
}
#endif
