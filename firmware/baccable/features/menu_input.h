#ifndef BACCABLE_MENU_INPUT_H
#define BACCABLE_MENU_INPUT_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    MENU_NONE,
    MENU_NEXT,
    MENU_PREVIOUS,
    MENU_NEXT_GROUP,
    MENU_PREVIOUS_GROUP,
    MENU_SELECT,
    MENU_BACK
} MenuEvent;
typedef struct {
    uint32_t started, last_seen;
    uint8_t button;
    bool armed, consumed;
} MenuInput;
MenuEvent menu_input_update(MenuInput *input, uint8_t button, bool allowed, uint32_t now);
#endif
