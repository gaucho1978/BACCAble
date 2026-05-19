/*
 * setup_menu.h
 *
 * Single source of truth for uint8_t parameters persisted to flash.
 * See setup_menu.c for the table and how to add a new entry.
 */

#ifndef INC_SETUP_MENU_H_
#define INC_SETUP_MENU_H_

#include <stdint.h>

#if defined(C1baccable)

#define SETUP_MENU_NO_PAGE  0xFF  // entry is not shown in the setup menu
#define SETUP_FLASH_SLOTS   30    // total flash slots (1..SETUP_FLASH_SLOTS)

// Describes one uint8_t parameter persisted to flash.
// Entries with menu_page != SETUP_MENU_NO_PAGE appear as checkboxes in the
// setup menu. max_value == 1 means simple on/off; higher values mean the
// toggle requires custom logic in the caller's switch-case.
typedef struct {
    uint8_t  flash_index;  // 1-based slot (matches readFromFlash argument)
    uint8_t  menu_page;    // page shown in the setup menu, or SETUP_MENU_NO_PAGE
    uint8_t  max_value;    // 1 = bool, >1 = multi-value (custom toggle)
    uint8_t *value;        // pointer to the runtime variable
} SetupParam;

extern const SetupParam setup_params[];
extern const uint8_t    setup_params_count;

// Load all table entries from flash into their runtime variables.
void setup_load_from_flash(void);

// Write each table entry into params[flash_index-1].
// Non-table slots (uint16_t and int8_t) must be filled by the caller.
void setup_fill_flash_params(uint16_t *params);

// Return 1 if any table entry differs from its stored flash value.
uint8_t setup_is_dirty(void);

// If page_index has a table entry, write checkbox_symbols[!!*value] into
// dashboard_setup_menu_array[page_index][0]. Returns 1 if handled.
uint8_t setup_update_checkbox(uint8_t page_index);

// If page_index maps to a max_value==1 entry, toggle *value and return 1.
// Returns 0 when the page needs custom toggle logic in the caller's switch.
uint8_t setup_toggle_if_bool(uint8_t page_index);

#endif /* C1baccable */

#endif /* INC_SETUP_MENU_H_ */
