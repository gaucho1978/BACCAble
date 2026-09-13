#ifndef BACCABLE_UI_ENTRY_H
#define BACCABLE_UI_ENTRY_H
#include <stdbool.h>
#include <stddef.h>

/* Entry semantics shared by settings, actions and read-only menu screens. */
typedef enum {
    UI_ENTRY_TOGGLE,
    UI_ENTRY_ENUM,
    UI_ENTRY_NUMBER,
    UI_ENTRY_ACTION,
    UI_ENTRY_CONDITIONAL_ACTION,
    UI_ENTRY_CAPTURE,
    UI_ENTRY_EXCLUSIVE_MODE,
    UI_ENTRY_SUBMENU,
    UI_ENTRY_STATUS
} UiEntryType;

/* Production glyphs are ASCII; unverified IPC bytes belong only in diagnostic builds. */
#define UI_SYMBOL_ENTER ">"
#define UI_SYMBOL_WARNING "!"
#define UI_SYMBOL_UNKNOWN "?"
#define UI_SYMBOL_SELECTED "*"
#define UI_SYMBOL_BACK "<"
#define UI_VALUE_ON "ON"
#define UI_VALUE_OFF "OFF"

/* Keep values intact; return false when the full entry needs the screen without a counter. */
bool ui_render_list_entry(char *text, size_t capacity, unsigned current, unsigned total,
                          const char *entry);
/* Return the space used by a valid position prefix, or zero for a non-list. */
size_t ui_render_position(char *text, size_t capacity, unsigned current, unsigned total);

void ui_render_toggle(char *text, size_t capacity, const char *label, bool enabled);
void ui_render_value(char *text, size_t capacity, const char *label, const char *value);
void ui_render_number(char *text, size_t capacity, const char *label, int value, bool editing);
void ui_render_signed_number(char *text, size_t capacity, const char *label, int value, bool editing);
void ui_render_action(char *text, size_t capacity, const char *label);
void ui_render_unavailable(char *text, size_t capacity, const char *reason);
void ui_render_pending(char *text, size_t capacity, const char *label, const char *request);
#endif
