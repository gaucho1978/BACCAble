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

/* Invalid or empty lists have no position; callers can show their empty-state text. */
size_t ui_render_position(char *text, size_t capacity, unsigned current, unsigned total) {
    if (!capacity)
        return 0;
    text[0] = 0;
    if (!current || current > total)
        return 0;
    int used = snprintf_(text, capacity, "%u/%u ", current, total);
    return used > 0 && (size_t)used < capacity ? (size_t)used : 0;
}

/* Shorten descriptive labels before values or the explicit action/back symbol. */
bool ui_render_list_entry(char *text, size_t capacity, unsigned current, unsigned total,
                          const char *entry) {
    size_t used = ui_render_position(text, capacity, current, total);
    if (!capacity)
        return false;
    char *out = text + used;
    size_t remaining = capacity - used;
    if (entry[0] == '!' && entry[1] == ' ' && used &&
        strlen(entry) >= remaining && strlen(entry + 2) < remaining) {
        text[used - 1] = '!';
        snprintf_(text + used, capacity - used, "%s", entry + 2);
        return true;
    }
    const char *value = strstr(entry, ": ");
    if (value) {
        size_t suffix = strlen(value);
        if (suffix >= remaining) {
            /* Never turn a long version, mode or status into a different value. */
            snprintf_(text, capacity, "%s", entry);
            return false;
        }
        int label = remaining > suffix + 1 ? (int)(remaining - suffix - 1) : 0;
        if (label > (int)(value - entry)) label = (int)(value - entry);
        snprintf_(out, remaining, "%.*s%s", label, entry, value);
    } else {
        size_t length = strlen(entry);
        if (length >= 2 && entry[length - 2] == ' ' && entry[length - 1] == '>') {
            size_t label = remaining > 3 ? remaining - 3 : 0;
            if (label > length - 2) label = length - 2;
            snprintf_(out, remaining, "> %.*s", (int)label, entry);
        } else
            snprintf_(out, remaining, "%s", entry);
    }
    return used != 0;
}
