#include "features/menu_model.h"
#include <string.h>
const char *const menu_group_names[MENU_GROUPS] = {"All readings", "Engine",      "Temperatures", "Battery",
                                                   "DPF / AdBlue", "Performance", "Other"};

/* Return the number of readings available for an engine profile. */
uint8_t menu_page_count(uint8_t engine) {
    return engine == 0 ? gasoline_page_count : engine == 1 ? diesel_page_count : 0;
}

/* Find a catalog page by its permanent identity. */
int menu_page_index(uint8_t engine, uint16_t id) {
    if (!id)
        return -1;
    for (unsigned i = 0; i < menu_page_count(engine); ++i)
        if (parameter_pages[engine][i].id == id)
            return (int)i;
    return -1;
}

/* Provide initial favorites and a visible parameter catalog. */
void menu_preferences_default(MenuPreferences *prefs) {
    memset(prefs, 0, sizeof(*prefs));
    const uint16_t favorites[2][4] = {{0x04, 0x01, 0x07, 0x28}, {0x84, 0x81, 0x87, 0x88}};
    for (unsigned e = 0; e < 2; ++e) {
        memcpy(prefs->favorites[e], favorites[e], sizeof(favorites[e]));
        prefs->last_favorite[e] = favorites[e][0];
    }
}

/* Add a permanent page identity to the saved menu preferences. */
static void put16(uint8_t *data, unsigned *offset, uint16_t value) {
    data[(*offset)++] = value;
    data[(*offset)++] = value >> 8;
}

/* Read a permanent page identity from saved menu preferences. */
static uint16_t get16(const uint8_t *data, unsigned *offset) {
    uint16_t value = data[(*offset)++];
    return value | (uint16_t)data[(*offset)++] << 8;
}

/* Prepare menu preferences for saving without depending on their in-memory layout. */
void menu_preferences_encode(const MenuPreferences *prefs, uint8_t data[MENU_PREFS_SIZE]) {
    memset(data, 0, MENU_PREFS_SIZE);
    data[0] = 1;
    data[1] = !!prefs->alphabetical;
    unsigned offset = 2;
    for (unsigned e = 0; e < 2; ++e) {
        for (unsigned i = 0; i < MENU_FAVORITES; ++i)
            put16(data, &offset, prefs->favorites[e][i]);
        for (unsigned i = 0; i < MENU_GROUPS; ++i)
            put16(data, &offset, prefs->last[e][i]);
        put16(data, &offset, prefs->last_favorite[e]);
        memcpy(data + offset, prefs->hidden[e], 8);
        offset += 8;
    }
}

/* Restore supported menu preferences and discard invalid or duplicate favorites. */
bool menu_preferences_decode(MenuPreferences *prefs, const uint8_t data[MENU_PREFS_SIZE]) {
    if (data[0] != 1 || data[1] > 1)
        return false;
    memset(prefs, 0, sizeof(*prefs));
    prefs->alphabetical = data[1];
    unsigned offset = 2;
    for (unsigned e = 0; e < 2; ++e) {
        unsigned n = 0;
        for (unsigned i = 0; i < MENU_FAVORITES; ++i) {
            uint16_t id = get16(data, &offset);
            bool duplicate = false;
            for (unsigned j = 0; j < n; ++j)
                duplicate |= prefs->favorites[e][j] == id;
            if (!duplicate && menu_page_index(e, id) >= 0)
                prefs->favorites[e][n++] = id;
        }
        for (unsigned i = 0; i < MENU_GROUPS; ++i)
            prefs->last[e][i] = get16(data, &offset);
        prefs->last_favorite[e] = get16(data, &offset);
        memcpy(prefs->hidden[e], data + offset, 8);
        offset += 8;
    }
    return true;
}

/* Check whether a page is enabled in the ordinary parameter catalog. */
bool menu_page_visible(const MenuPreferences *prefs, uint8_t engine, uint8_t index) {
    if (index >= menu_page_count(engine))
        return false;
    unsigned slot = (parameter_pages[engine][index].id & 0x7f) - 1;
    return slot < 64 && !(prefs->hidden[engine][slot / 8] & (1U << (slot % 8)));
}

/* Change a page's visibility without changing its favorite status. */
void menu_page_show(MenuPreferences *prefs, uint8_t engine, uint8_t index, bool visible) {
    if (index >= menu_page_count(engine))
        return;
    unsigned slot = (parameter_pages[engine][index].id & 0x7f) - 1;
    if (slot >= 64)
        return;
    uint8_t mask = 1U << (slot % 8);
    if (visible)
        prefs->hidden[engine][slot / 8] &= (uint8_t)~mask;
    else
        prefs->hidden[engine][slot / 8] |= mask;
}

/* Build the requested parameter or favorite list in the chosen order. */
unsigned menu_page_list(const MenuPreferences *prefs, uint8_t engine, uint8_t group, bool favorites,
                        bool include_hidden, uint8_t list[60]) {
    unsigned count = 0;
    if (engine > 1 || group >= MENU_GROUPS)
        return 0;
    if (favorites) {
        for (unsigned i = 0; i < MENU_FAVORITES; ++i) {
            int index = menu_page_index(engine, prefs->favorites[engine][i]);
            /* Favorites and visibility are independent choices. */
            if (index >= 0)
                list[count++] = (uint8_t)index;
        }
        return count;
    }
    for (unsigned i = 0; i < menu_page_count(engine); ++i) {
        const ParameterPage *page = &parameter_pages[engine][i];
        if ((!group || page->group == group) && (include_hidden || menu_page_visible(prefs, engine, i)))
            list[count++] = i;
    }
    for (unsigned i = 1; i < count; ++i) {
        uint8_t item = list[i];
        unsigned j = i;
        while (j) {
            const ParameterPage *a = &parameter_pages[engine][list[j - 1]];
            const ParameterPage *b = &parameter_pages[engine][item];
            bool after = prefs->alphabetical ? strcmp(a->label, b->label) > 0 : a->group > b->group;
            if (!after)
                break;
            list[j] = list[j - 1];
            --j;
        }
        list[j] = item;
    }
    return count;
}

/* Add or remove a favorite while respecting the six-page limit. */
bool menu_favorite_toggle(MenuPreferences *prefs, uint8_t engine, uint16_t id) {
    if (menu_page_index(engine, id) < 0)
        return false;
    uint16_t *items = prefs->favorites[engine];
    for (unsigned i = 0; i < MENU_FAVORITES; ++i) {
        if (items[i] == id) {
            memmove(items + i, items + i + 1, (MENU_FAVORITES - i - 1) * sizeof(*items));
            items[MENU_FAVORITES - 1] = 0;
            return true;
        }
        if (!items[i]) {
            items[i] = id;
            return true;
        }
    }
    return false;
}

/* Move a favorite one position within the user's custom order. */
void menu_favorite_move(MenuPreferences *prefs, uint8_t engine, uint16_t id, int direction) {
    if (engine > 1)
        return;
    uint16_t *items = prefs->favorites[engine];
    for (unsigned i = 0; i < MENU_FAVORITES; ++i) {
        int next = (int)i + (direction < 0 ? -1 : 1);
        if (items[i] == id && next >= 0 && next < MENU_FAVORITES && items[next]) {
            items[i] = items[next];
            items[next] = id;
            return;
        }
    }
}
