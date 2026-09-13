#ifndef BACCABLE_UI_GLYPHS_H
#define BACCABLE_UI_GLYPHS_H

/* Raw single-byte glyphs verified on the user's IPC; other revisions need validation.
 * Never substitute UTF-8 or Windows-1252 characters from the 0x80-0x9F range. */
#define UI_GLYPH_UNCHECKED ((char)0x4F)
#define UI_GLYPH_CHECKED ((char)0xD8)
#define UI_GLYPH_PREV ((char)0xAB)
#define UI_GLYPH_NEXT ((char)0xBB)
#define UI_GLYPH_DEGREE ((char)0xB0)
#define UI_GLYPH_DOT ((char)0xB7)
#define UI_GLYPH_CROSS ((char)0xD7)
#define UI_GLYPH_PLUSMINUS ((char)0xB1)
/* String form for failure messages composed at compile time. */
#define UI_SYMBOL_FAILURE "\xD7"

#endif
