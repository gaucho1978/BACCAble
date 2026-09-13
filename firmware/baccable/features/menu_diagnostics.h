#ifndef BACCABLE_MENU_DIAGNOSTICS_H
#define BACCABLE_MENU_DIAGNOSTICS_H

#include <stddef.h>

/* Explicit development build only; never part of the production menu. */
#ifdef MENU_DIAGNOSTICS
void menu_diagnostics_reset(void);
/* NEXT/PREVIOUS change the glyph group or display pattern, with wrapping. */
void menu_diagnostics_move(int direction);
/* SELECT cycles between the character map and display refresh patterns. */
void menu_diagnostics_select(void);
/* capacity includes the terminating NUL; output fits an 18-character IPC. */
void menu_diagnostics_render(char *text, size_t capacity);
#endif

#endif
