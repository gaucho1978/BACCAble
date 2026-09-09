#include "app/powertrain.h"
#include "features/value_format.h"
#if defined(BACCABLE_C1)
uint8_t parameter_page_find(uint32_t searchedReqId) {

    for (uint8_t i = 0; i < parameter_page_count; i++) {
        if (parameter_definitions[parameter_pages[settings_state.is_diesel_enabled][i].parameter_ids[0]]
                .request_id == searchedReqId)
            return i;
    }
    // if not found, search it in second param
    for (uint8_t i = 0; i < parameter_page_count; i++) {
        if (parameter_definitions[parameter_pages[settings_state.is_diesel_enabled][i].parameter_ids[1]]
                .request_id == searchedReqId)
            return i;
    }
    return 0; // means not found, or param0 (it could be an exception)
}

uint8_t parameter_page_next(uint8_t start) {
    return visible_page_find(parameter_page_visibility, parameter_page_count, start, 1);
}

uint8_t parameter_page_previous(uint8_t start) {
    return visible_page_find(parameter_page_visibility, parameter_page_count, start, -1);
}
#endif
