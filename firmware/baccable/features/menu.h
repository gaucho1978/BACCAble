#ifndef BACCABLE_MENU_H
#define BACCABLE_MENU_H
#include "features/menu_input.h"
#include "features/menu_model.h"
void menu_init(void);
void menu_peer_status(uint8_t peer, const uint8_t *version);
void menu_process(void);
void menu_action_reply(uint8_t command);
bool menu_setting_busy(uint8_t flash_index);
void menu_button(uint8_t button, bool allowed);
void menu_event(MenuEvent event);
void menu_render(void);
void menu_notice(const char *text);
void menu_present(const char *text);
void menu_present_reading(const char *text);
bool menu_parameters_active(void);
void menu_show_parameter(uint8_t index);
uint8_t menu_preferences_save(void);
void menu_engine_changed(void);
void menu_parameters_refresh(void);
#endif
