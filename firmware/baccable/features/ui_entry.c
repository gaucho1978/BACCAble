#include "features/ui_entry.h"
#include "third_party/printf/printf.h"
#include <string.h>

/* Retain the value when a compact display requires shortening the label. */
void ui_render_value(char *text, size_t capacity, const char *label, const char *value) {
    size_t suffix = strlen(value) + 3;
    int length = capacity > suffix ? (int)(capacity - suffix) : 0;
    if (!length)
        snprintf_(text, capacity, "%s", value);
    else
        snprintf_(text, capacity, "%.*s: %s", length, label, value);
}

/* Use the same explicit boolean words for all true switches. */
void ui_render_toggle(char *text, size_t capacity, const char *label, bool enabled) {
    ui_render_value(text, capacity, label, enabled ? UI_VALUE_ON : UI_VALUE_OFF);
}

/* Mark an editable draft without changing the meaning of its numeric value. */
void ui_render_number(char *text, size_t capacity, const char *label, int value, bool editing) {
    char number[16];
    snprintf_(number, sizeof(number), "%d", value);
    if (editing && capacity > 2) {
        text[0] = UI_SYMBOL_SELECTED[0];
        text[1] = ' ';
        ui_render_value(text + 2, capacity - 2, label, number);
    } else
        ui_render_value(text, capacity, label, number);
}

/* Preserve the sign on trim values so positive and negative adjustments are explicit. */
void ui_render_signed_number(char *text, size_t capacity, const char *label, int value, bool editing) {
    char number[16];
    snprintf_(number, sizeof(number), "%+d", value);
    if (editing && capacity > 2) {
        text[0] = UI_SYMBOL_SELECTED[0];
        text[1] = ' ';
        ui_render_value(text + 2, capacity - 2, label, number);
    } else
        ui_render_value(text, capacity, label, number);
}

/* Distinguish commands and workflows from editable values. */
void ui_render_action(char *text, size_t capacity, const char *label) {
    int length = capacity > 3 ? (int)(capacity - 3) : 0;
    snprintf_(text, capacity, "%.*s " UI_SYMBOL_ENTER, length, label);
}

/* Explain a known condition instead of offering an apparently usable action. */
void ui_render_unavailable(char *text, size_t capacity, const char *reason) {
    snprintf_(text, capacity, UI_SYMBOL_WARNING " %s", reason);
}

/* Explicitly identify an unresolved request without claiming physical success. */
void ui_render_pending(char *text, size_t capacity, const char *label, const char *request) {
    char value[32];
    snprintf_(value, sizeof(value), "%s%sWAIT", request, request[0] ? " " : "");
    ui_render_value(text, capacity, label, value);
}
