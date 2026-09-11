#ifndef BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H
#define BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H
#include "app/build_config.h"
#include "stm32f0xx_hal.h"

// this is used to invert bytes order in a 32 bit integer
#define SWAP_UINT32(x)                                                                                       \
    (((uint32_t)(x) >> 24) & 0x000000FF) | (((uint32_t)(x) >> 8) & 0x0000FF00) |                             \
        (((uint32_t)(x) << 8) & 0x00FF0000) | (((uint32_t)(x) << 24) & 0xFF000000)
typedef struct {
    uint8_t id; /* Stable ID, independent of table position: never reuse. */
    uint8_t group;
    uint8_t parameter_ids[4];
    uint8_t element_count; /* Zero preserves the original one/two-reading pages. */
    const char *label;
    const char *name;
} ParameterPage;

typedef struct {
    /* Group naturally aligned fields together; no packed/unaligned accesses. */
    uint32_t request_id;
    uint32_t request_data;
    uint32_t response_id;
    int32_t raw_offset;
    float scale;
    int32_t scaled_offset;
    uint8_t request_length;
    uint8_t value_length;
    uint8_t value_offset;
} ParameterDefinition;

/* Return the number of measurements requested by this screen. */
static inline unsigned parameter_page_elements(const ParameterPage *page) {
    return page->element_count ? page->element_count : 2;
}

extern float displayed_parameter_values[4];
extern uint8_t parameter_page_count;
extern uint8_t gasoline_page_count;
extern uint8_t diesel_page_count;
extern const ParameterPage parameter_pages[2][64];
extern const ParameterDefinition parameter_definitions[100];
extern uint8_t selected_parameter_element;

extern const char *regeneration_labels[];
extern const char *seatbelt_labels[];
extern const uint8_t gear_symbols[11];
extern const char *statistics_labels[];

#endif /* BACCABLE_DIAGNOSTICS_PARAMETER_CATALOG_H */
