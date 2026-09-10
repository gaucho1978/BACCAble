#ifndef BACCABLE_APP_POWERTRAIN_H
#define BACCABLE_APP_POWERTRAIN_H
#include "app/application_state.h"
#include "platform/system.h"
#include "settings/setup_menu.h"

void powertrain_init(void);
void pedal_booster_set_map(uint8_t map);
void powertrain_process(void);
uint8_t pedal_booster_needs_update(void);

void dashboard_send_setup(void);
void dashboard_send_values(void);
float native_parameter_read(uint8_t paramId);
void dashboard_format_values(const char *template, float values[2], const uint8_t paramId[2], char *result);
uint32_t system_free_ram(void);
void dashboard_clear(void);
float accelerator_percent(uint8_t vol);
uint8_t gear_color(uint8_t col);
uint8_t settings_save(void);
uint8_t statistics_reset();
uint8_t statistics_save_best(void);
uint16_t settings_read(uint8_t paramId);
float statistics_read_best(uint8_t paramId);
uint8_t parameter_page_find(uint32_t searchedReqId);
#endif /* BACCABLE_APP_POWERTRAIN_H */
