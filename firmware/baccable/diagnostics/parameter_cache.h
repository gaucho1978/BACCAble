#ifndef BACCABLE_PARAMETER_CACHE_H
#define BACCABLE_PARAMETER_CACHE_H
#include <stdint.h>
#include <stdbool.h>
void parameter_peak_reset(void);
void parameter_peak_enable(bool enabled);
bool parameter_peak_enabled(void);
void parameter_cache_reset(void);
void parameter_cache_put(uint8_t id, float value, uint32_t now);
float parameter_cache_get(uint8_t id, uint32_t now);
void parameter_cache_observe(uint32_t id, uint8_t length);
#endif
