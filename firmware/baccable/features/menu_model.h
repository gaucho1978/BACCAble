#ifndef BACCABLE_MENU_MODEL_H
#define BACCABLE_MENU_MODEL_H
#include "diagnostics/parameter_catalog.h"
#include <stdbool.h>
#define MENU_FAVORITES 6
#define MENU_GROUPS 7
#define MENU_PREFS_SIZE 80
typedef struct {
    uint16_t favorites[2][MENU_FAVORITES];
    uint16_t last[2][MENU_GROUPS];
    uint16_t last_favorite[2];
    uint8_t hidden[2][8];
    uint8_t alphabetical;
} MenuPreferences;
extern const char *const menu_group_names[MENU_GROUPS];
uint8_t menu_page_count(uint8_t engine);
int menu_page_index(uint8_t engine, uint16_t id);
void menu_preferences_default(MenuPreferences *prefs);
void menu_preferences_encode(const MenuPreferences *prefs, uint8_t data[MENU_PREFS_SIZE]);
bool menu_preferences_decode(MenuPreferences *prefs, const uint8_t data[MENU_PREFS_SIZE]);
bool menu_page_visible(const MenuPreferences *prefs, uint8_t engine, uint8_t index);
void menu_page_show(MenuPreferences *prefs, uint8_t engine, uint8_t index, bool visible);
unsigned menu_page_list(const MenuPreferences *prefs, uint8_t engine, uint8_t group, bool favorites,
                        bool include_hidden, uint8_t list[64]);
bool menu_favorite_toggle(MenuPreferences *prefs, uint8_t engine, uint16_t id);
void menu_favorite_move(MenuPreferences *prefs, uint8_t engine, uint16_t id, int direction);
#endif
