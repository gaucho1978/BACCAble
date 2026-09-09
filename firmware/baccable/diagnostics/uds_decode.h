#ifndef BACCABLE_DIAGNOSTICS_UDS_DECODE_H
#define BACCABLE_DIAGNOSTICS_UDS_DECODE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool uds_decode_value(const uint8_t *data, size_t length, uint16_t did, uint8_t offset, uint8_t width,
                      int32_t raw_offset, float scale, int32_t scaled_offset, float *value);
#endif
