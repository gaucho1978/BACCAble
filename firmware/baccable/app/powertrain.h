#ifndef BACCABLE_APP_POWERTRAIN_H
#define BACCABLE_APP_POWERTRAIN_H
#include "app/application_state.h"
#include "platform/system.h"
#include "settings/setup_menu.h"

void powertrain_init(void);
void pedal_booster_set_map(uint8_t map);
void powertrain_process(void);
uint8_t pedal_booster_needs_update(void);

void dashboard_send_main(void);
void dashboard_send_setup(void);
void dashboard_send_parameter_setup(void);
void dashboard_send_values(void);
float native_parameter_read(uint8_t paramId);
void dashboard_format_values(const char *template, float values[2], const uint8_t paramId[2], char *result);
uint8_t dashboard_remove_placeholders(char *str);
uint32_t system_free_ram(void);
void dashboard_clear(void);
float accelerator_percent(uint8_t vol);
uint8_t gear_color(uint8_t col);
uint8_t settings_save(void);
uint8_t statistics_reset();
uint8_t statistics_save_best(void);
uint8_t visibility_save(void);
void compress_to_uint16(const uint8_t *input, size_t input_len, uint16_t *output);
void decompress_from_uint16(const uint16_t *input, size_t input_len, uint8_t *output, size_t output_len);
void visibility_load(void);
uint16_t settings_read(uint8_t paramId);
float statistics_read_best(uint8_t paramId);
uint8_t parameter_page_find(uint32_t searchedReqId);
uint8_t parameter_page_next(uint8_t curIndex);
uint8_t parameter_page_previous(uint8_t curIndex);
#endif /* BACCABLE_APP_POWERTRAIN_H */
