#include "app/powertrain.h"
#if defined(BACCABLE_C1)

/* Find a screen containing the requested measurement for an automatic result view. */
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

#endif
