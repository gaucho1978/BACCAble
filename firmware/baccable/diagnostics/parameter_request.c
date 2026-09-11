#include "app/powertrain.h"
#include "diagnostics/parameter_request.h"
#include "diagnostics/uds_decode.h"
#include "diagnostics/parameter_cache.h"
#include "features/menu.h"
#if defined(BACCABLE_C1)
static struct {
    uint8_t active, engine, page, element, parameter;
    uint32_t started;
} request;

/* Discard a pending reading when its screen is no longer relevant. */
void parameter_request_cancel(void) { request.active = 0; }

/* Ask the vehicle for the diagnostic value selected on the current screen. */
void parameter_request_begin(void) {
    if (request.active && currentTime - request.started < 500)
        return;
    request.active = 0;
    if (settings_state.is_diesel_enabled > 1 ||
        dashboard_state.dashboard_page_index >= parameter_page_count ||
        selected_parameter_element >=
            parameter_page_elements(
                &parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index]))
        return;
    uint8_t id = parameter_pages[settings_state.is_diesel_enabled][dashboard_state.dashboard_page_index]
                     .parameter_ids[selected_parameter_element];
    if (id >= 100)
        return;
    const ParameterDefinition *parameter = &parameter_definitions[id];
    if (parameter->request_id <= 0xff || parameter->request_length != 4)
        return;
    uint8_t data[8] = {0};
    for (unsigned i = 0; i < 4; ++i)
        data[i] = parameter->request_data >> (8 * i);
    CAN_TxHeaderTypeDef header = {
        .IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .ExtId = parameter->request_id, .DLC = 4};
    if (can_tx(&header, data) != HAL_OK)
        return;
    request.engine = settings_state.is_diesel_enabled;
    request.page = dashboard_state.dashboard_page_index;
    request.element = selected_parameter_element;
    request.parameter = id;
    request.started = currentTime;
    request.active = 1;
}

/* Accept a matching diagnostic reply and update the current reading. */
void parameter_request_receive(const CAN_RxHeaderTypeDef *header, const uint8_t *data) {
    if (!request.active)
        return;
    if (currentTime - request.started > 500 || !dashboard_state.baccable_dashboard_menu_visible ||
        !menu_parameters_active() || settings_state.is_diesel_enabled != request.engine ||
        dashboard_state.dashboard_page_index != request.page) {
        request.active = 0;
        return;
    }
    const ParameterDefinition *parameter = &parameter_definitions[request.parameter];
    uint16_t did = ((parameter->request_data >> 16) & 0xff) << 8 | (parameter->request_data >> 24);
    float value;
    if (header->ExtId != parameter->response_id ||
        !uds_decode_value(data, header->DLC, did, parameter->value_offset, parameter->value_length,
                          parameter->raw_offset, parameter->scale, parameter->scaled_offset, &value))
        return;
    parameter_cache_put(request.parameter, value, currentTime);
    displayed_parameter_values[request.element] = value;
    request.active = 0;
    menu_render();
}
#endif
