#include "diagnostics/parameter_catalog.h"
#include "app/build_config.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(ParameterDefinition) == 28, "Parameter catalog must retain natural compact layout");

/* Budget the fully expanded screen, including units and the longest enum states. */
static void check_screen_width(const ParameterPage *page) {
    assert(strlen(page->label) <= 16); /* Two columns reserved for editor status. */
    for (const char *s = page->label; *s; ++s)
        assert(*s >= 32 && *s <= 126);
    unsigned width = 0, element = 0;
    for (const char *s = page->name; *s;) {
        if (*s != '$') {
            ++width;
            ++s;
            continue;
        }
        assert(element < parameter_page_elements(page) && strlen(s) >= 5);
        if (!strncmp(s, "$enum", 5)) {
            uint8_t id = page->parameter_ids[element];
            width += id == 8 ? 10 : id == 13 ? 3 : 2;
        } else {
            assert(s[1] >= '0' && s[1] <= '9' && s[2] == '.' && s[3] >= '0' && s[3] <= '9' && s[4] == 'f');
            width += s[1] - '0' + s[3] - '0' + (s[3] != '0');
        }
        ++element;
        s += 5;
    }
    assert(width <= DASHBOARD_MESSAGE_MAX_LENGTH);
}
int main(void) {
    const uint8_t counts[] = {gasoline_page_count, diesel_page_count};
    for (unsigned engine = 0; engine < 2; ++engine) {
        assert(counts[engine] <= 64);
        for (unsigned page = 0; page < 64; ++page) {
            const ParameterPage *entry = &parameter_pages[engine][page];
            if (page < counts[engine])
                assert(entry->id == (engine ? 0x81 : 0x01) + page); /* Persisted physical order. */
            assert((entry->name != NULL) == (page < counts[engine]));
            if (page >= counts[engine])
                continue;
            check_screen_width(entry);
            for (unsigned element = 0; element < parameter_page_elements(entry); ++element) {
                assert(entry->parameter_ids[element] < 100);
                const ParameterDefinition *parameter = &parameter_definitions[entry->parameter_ids[element]];
                assert(parameter->request_id);
                assert(isfinite(parameter->scale));
                assert(parameter->value_length <= 4);
                if (parameter->request_id > 0xff) {
                    assert(parameter->request_length == 4);
                    assert((parameter->request_data & 0xffff) == 0x2203);
                    assert(parameter->response_id <= 0x1fffffff);
                }
            }
        }
    }
    assert(fabsf(parameter_definitions[79].scale - 0.001f) < 1e-8f);
    assert(fabsf(parameter_definitions[81].scale - 0.001f) < 1e-8f);
    assert(parameter_definitions[81].raw_offset == -32768);
    puts("PASS: gasoline/diesel page counts, parameter references, request schema, signed scaling");
    return 0;
}
