#include "diagnostics/parameter_catalog.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const uint8_t counts[] = {gasoline_page_count, diesel_page_count};
    for (unsigned engine = 0; engine < 2; ++engine) {
        assert(counts[engine] <= 60);
        for (unsigned page = 0; page < 60; ++page) {
            const ParameterPage *entry = &parameter_pages[engine][page];
            assert(memchr(entry->name, 0, sizeof(entry->name)));
            assert((entry->name[0] != 0) == (page < counts[engine]));
            if (page >= counts[engine])
                continue;
            for (unsigned element = 0; element < 2; ++element) {
                assert(entry->parameter_ids[element] < 100);
                const ParameterDefinition *parameter = &parameter_definitions[entry->parameter_ids[element]];
                assert(parameter->request_id);
                assert(isfinite(parameter->scale));
                assert(parameter->value_length <= 4 && parameter->decimal_places <= 6);
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
