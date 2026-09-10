#include "features/menu_input.h"
#include <string.h>

/* Recognize one deliberate navigation gesture, including hold-to-return behavior. */
MenuEvent menu_input_update(MenuInput *input, uint8_t button, bool allowed, uint32_t now) {
    if (!allowed) {
        memset(input, 0, sizeof(*input));
        return MENU_NONE;
    }
    if (input->armed && now - input->last_seen > 300) {
        input->armed = false; /* A lost release must never become an action. */
        input->button = 0;
    }
    input->last_seen = now;
    if (button == 0x50)
        button = 0x90;
    if (!input->armed) {
        if (button == 0x10) {
            input->armed = true;
            input->button = button;
        }
        return MENU_NONE;
    }
    if (button == 0x10) {
        bool select = input->button == 0x90 && !input->consumed;
        input->button = button;
        input->consumed = false;
        return select ? MENU_SELECT : MENU_NONE;
    }
    if (button != input->button) {
        uint8_t previous = input->button;
        input->button = button;
        input->started = now;
        input->consumed = false;
        if (button == 0x20 && (previous == 0x18 || previous == 0x10))
            return MENU_NEXT_GROUP;
        if (button == 0x00 && (previous == 0x08 || previous == 0x10))
            return MENU_PREVIOUS_GROUP;
        if (previous == 0x10 && button == 0x18)
            return MENU_NEXT;
        if (previous == 0x10 && button == 0x08)
            return MENU_PREVIOUS;
        return MENU_NONE;
    }
    if (button == 0x90 && !input->consumed && now - input->started >= 800) {
        input->consumed = true;
        return MENU_BACK;
    }
    return MENU_NONE;
}
