#ifndef BACCABLE_FEATURES_VALUE_FORMAT_H
#define BACCABLE_FEATURES_VALUE_FORMAT_H
#include <stddef.h>
#include <stdint.h>
void format_number(char *text, float value, uint8_t precision, uint8_t capacity);
uint8_t visible_page_find(const uint8_t *visible, uint8_t count, uint8_t start, int direction);
#endif
