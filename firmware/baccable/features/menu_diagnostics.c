#include "features/menu_diagnostics.h"

#ifdef MENU_DIAGNOSTICS
#include <stdbool.h>
#include <stdint.h>

enum { CHARSET_GROUPS = 28 };
static uint8_t group;
static bool display_test;
static bool alternate_pattern;

/* Start the hidden test at the first printable ASCII range. */
void menu_diagnostics_reset(void) {
    group = 0;
    display_test = false;
    alternate_pattern = false;
}

/* Browse raw glyph groups or alternate the refresh pattern. */
void menu_diagnostics_move(int direction) {
    if (direction == 0) return;
    if (display_test) {
        alternate_pattern = !alternate_pattern;
    } else if (direction > 0) {
        group = (uint8_t)((group + 1) % CHARSET_GROUPS);
    } else {
        group = group == 0 ? CHARSET_GROUPS - 1 : group - 1;
    }
}

/* Switch between character inspection and the refresh test. */
void menu_diagnostics_select(void) {
    display_test = !display_test;
}

/* Label raw bytes in hex without assuming a dashboard character encoding. */
void menu_diagnostics_render(char *text, size_t capacity) {
    static const char hex[] = "0123456789ABCDEF";
    static const char patterns[2][19] = {"Display A:12345678", "Display B:87654321"};
    char line[19];
    size_t length;
    if (!text || capacity == 0) return;

    if (display_test) {
        length = 18;
        for (size_t i = 0; i < length; ++i) line[i] = patterns[alternate_pattern][i];
    } else {
        uint8_t first = group < 12 ? (uint8_t)(0x20 + group * 8)
                                   : (uint8_t)(0x80 + (group - 12) * 8);
        uint8_t count = group == 11 ? 7 : 8;
        uint8_t last = (uint8_t)(first + count - 1);
        line[0] = hex[first >> 4];
        line[1] = hex[first & 15];
        line[2] = '-';
        line[3] = hex[last >> 4];
        line[4] = hex[last & 15];
        line[5] = ' ';
        for (uint8_t i = 0; i < count; ++i) line[6 + i] = (char)(first + i);
        length = 6 + count;
    }
    if (length >= capacity) length = capacity - 1;
    for (size_t i = 0; i < length; ++i) text[i] = line[i];
    text[length] = '\0';
}
#endif
