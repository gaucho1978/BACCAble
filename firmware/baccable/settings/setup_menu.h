/*
 * setup_menu.h
 *
 * Setup parameter contract. See setup_menu_entries.c for the table and how to
 * add a new entry.
 */

#ifndef INC_SETUP_MENU_H_
#define INC_SETUP_MENU_H_

#include <stdint.h>
#include "app/build_config.h"

#if defined(BACCABLE_C1)

    #define SETUP_FLASH_SLOTS 30 // minimum reserved flash slots (1..SETUP_FLASH_SLOTS)
    #define SETUP_FLASH_PARAM_BUFFER_SIZE 40
extern uint8_t setup_dashboardPageIndex;
extern uint8_t dashboard_setup_menu_array[SETUP_FLASH_PARAM_BUFFER_SIZE][DASHBOARD_MESSAGE_MAX_LENGTH];
extern uint8_t total_pages_in_setup_dashboard_menu;

typedef enum {
    SETUP_VALUE_UINT8 = 0,
    SETUP_VALUE_UINT16,
    SETUP_VALUE_INT8_AS_UINT8,
} SetupValueType;

typedef enum {
    SETUP_DISPLAY_NONE = 0,
    SETUP_DISPLAY_STATUS_MARK,
} SetupDisplayMode;

typedef void (*SetupRenderFn)(uint8_t page_index);
typedef void (*SetupActionFn)(void);

// Describes one parameter persisted to flash and optionally shown in setup.
// Keep setup behavior in this table: storage, defaults, display and action.
typedef struct {
    uint8_t flash_index;           // 1-based slot (matches settings_read argument)
    uint16_t max_value;            // highest valid persisted value
    uint16_t default_value;        // used when flash is empty or invalid
    SetupValueType value_type;     // runtime variable type
    SetupDisplayMode display_mode; // how the shared menu code may update display
    void *value;                   // pointer to the runtime variable
    const char *menu_text;         // text shown in setup menu; NULL text hides entry
    SetupRenderFn render;          // optional per-page display update
    SetupActionFn action;          // optional select action; NULL toggles bool
} SetupParam;

extern const SetupParam setup_params[];
extern const uint8_t setup_params_count;

const SetupParam *setup_find_by_flash_index(uint8_t flash_index);

// Return the number of flash slots that must be written to persist all table
// entries, including future entries above SETUP_FLASH_SLOTS.
uint8_t setup_flash_slots_count(void);

// Load all table entries from flash into their runtime variables.
void setup_load_from_flash(void);

// Write each table entry into params[flash_index-1].
void setup_fill_flash_params(uint16_t *params);

// Return 1 if any table entry differs from its stored flash value.
uint8_t setup_is_dirty(void);

// Return a validated value from flash or the table default for an invalid slot.
uint16_t setup_read_flash_value(uint8_t flash_index, uint16_t stored_value);

// Render one setup page into dashboard_setup_menu_array.
void setup_render_page(uint8_t page_index);

// Move current setup page by delta and wrap inside the setup menu.
void setup_move_page(int8_t delta);

// Execute the action for the selected setup page. Handles SAVE&EXIT too.
void setup_select_page(uint8_t page_index);

#endif /* BACCABLE_C1 */

#endif /* INC_SETUP_MENU_H_ */
