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

/* Reserve checked/unchecked for actual editable boolean choices, never vehicle state. */
void ui_render_checkbox(char *text, size_t capacity, const char *label, bool checked) {
    snprintf_(text, capacity, "%c %s", checked ? UI_GLYPH_CHECKED : UI_GLYPH_UNCHECKED, label);
}

/* Report a failed local operation without confusing it with an unmet precondition. */
void ui_render_failure(char *text, size_t capacity, const char *reason) {
    snprintf_(text, capacity, UI_SYMBOL_FAILURE " %s", reason);
}

/* Add editor direction hints only when the entire existing value and label still fit. */
static void editor_directions(char *text, size_t capacity) {
    size_t length = strlen(text);
    char *value = strstr(text, ": ");
    if (!value || length + 4 >= capacity)
        return;
    value += 2;
    memmove(value + 2, value, strlen(value) + 1);
    value[0] = UI_GLYPH_PREV;
    value[1] = ' ';
    text[length + 2] = ' ';
    text[length + 3] = UI_GLYPH_NEXT;
    text[length + 4] = 0;
}

/* Retain explicit words for status-only and vehicle-request state. */
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
        editor_directions(text, capacity);
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
        editor_directions(text, capacity);
    } else
        ui_render_value(text, capacity, label, number);
}

/* Distinguish commands and workflows from editable values. */
void ui_render_action(char *text, size_t capacity, const char *label) {
    int length = capacity > 3 ? (int)(capacity - 3) : 0;
    snprintf_(text, capacity, UI_SYMBOL_ENTER " %.*s", length, label);
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
