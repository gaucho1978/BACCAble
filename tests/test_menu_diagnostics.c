#include "features/menu_diagnostics.h"
#include "test_report.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void check_capacity(const char *expected) {
    /* Includes both IPC widths (18/24 plus NUL) and truncated buffers. */
    for (size_t capacity = 0; capacity <= 25; ++capacity) {
        unsigned char guarded[27];
        memset(guarded, 0xa5, sizeof(guarded));
        menu_diagnostics_render((char *)&guarded[1], capacity);
        assert(guarded[0] == 0xa5);
        for (size_t i = capacity + 1; i < sizeof(guarded); ++i)
            assert(guarded[i] == 0xa5);
        if (capacity) {
            size_t length = strlen(expected);
            if (length >= capacity) length = capacity - 1;
            assert(memcmp(&guarded[1], expected, length) == 0);
            assert(guarded[length + 1] == 0);
        }
    }
    menu_diagnostics_render(NULL, 0);
    menu_diagnostics_render(NULL, 19);
}

static void charset_bytes(void) {
    static const char hex[] = "0123456789ABCDEF";
    bool seen[256] = {false};
    char text[25];
    menu_diagnostics_reset();
    for (unsigned page = 0; page < 28; ++page) {
        unsigned first = page < 12 ? 0x20 + page * 8 : 0x80 + (page - 12) * 8;
        unsigned count = page == 11 ? 7 : 8;
        unsigned last = first + count - 1;
        menu_diagnostics_render(text, sizeof(text));
        assert(strlen(text) == 6 + count);
        assert(text[0] == hex[first >> 4] && text[1] == hex[first & 15]);
        assert(text[2] == '-' && text[5] == ' ');
        assert(text[3] == hex[last >> 4] && text[4] == hex[last & 15]);
        for (unsigned i = 0; i < 6; ++i) assert((uint8_t)text[i] < 0x80);
        for (unsigned i = 0; i < count; ++i) {
            unsigned byte = (uint8_t)text[6 + i];
            assert(byte == first + i);
            assert(!seen[byte]);
            seen[byte] = true;
        }
        check_capacity(text);
        menu_diagnostics_move(1);
    }
    for (unsigned byte = 0; byte < 256; ++byte)
        assert(seen[byte] == ((byte >= 0x20 && byte <= 0x7e) || byte >= 0x80));
    menu_diagnostics_render(text, sizeof(text));
    assert(strncmp(text, "20-27 ", 6) == 0);
    menu_diagnostics_move(-1);
    menu_diagnostics_move(0);
    menu_diagnostics_render(text, sizeof(text));
    assert(strncmp(text, "F8-FF ", 6) == 0);
}

static void display_patterns(void) {
    char text[25];
    menu_diagnostics_reset();
    menu_diagnostics_select();
    menu_diagnostics_render(text, sizeof(text));
    assert(strcmp(text, "Display A:12345678") == 0);
    check_capacity(text);
    menu_diagnostics_move(1);
    menu_diagnostics_render(text, sizeof(text));
    assert(strcmp(text, "Display B:87654321") == 0);
    check_capacity(text);
    menu_diagnostics_move(-1);
    menu_diagnostics_render(text, sizeof(text));
    assert(strcmp(text, "Display A:12345678") == 0);
    menu_diagnostics_select();
    menu_diagnostics_render(text, sizeof(text));
    assert(strncmp(text, "20-27 ", 6) == 0);
    menu_diagnostics_select();
    menu_diagnostics_move(1);
    menu_diagnostics_reset();
    menu_diagnostics_render(text, sizeof(text));
    assert(strncmp(text, "20-27 ", 6) == 0);
}

int main(void) {
    const HostTest tests[] = {HOST_TEST(charset_bytes), HOST_TEST(display_patterns)};
    host_tests_run("menu_diagnostics", tests, sizeof(tests) / sizeof(tests[0]));
    return 0;
}
