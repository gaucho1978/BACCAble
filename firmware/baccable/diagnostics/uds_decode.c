#include "diagnostics/uds_decode.h"

/* Convert a valid diagnostic reply into its displayed measurement. */
bool uds_decode_value(const uint8_t *data, size_t length, uint16_t did, uint8_t offset, uint8_t width,
                      int32_t raw_offset, float scale, int32_t scaled_offset, float *value) {
    if (!data || !value || length < 4 || length > 8 || width == 0 || width > 4)
        return false;
    size_t end = 4U + offset + width;
    /* Only a complete ISO-TP single frame can carry these catalog values. */
    if (data[0] < 3 || data[0] > 7 || (size_t)data[0] + 1 > length || end > (size_t)data[0] + 1 ||
        data[1] != 0x62 || data[2] != (uint8_t)(did >> 8) || data[3] != (uint8_t)did)
        return false;
    uint32_t raw = 0;
    for (size_t i = 4U + offset; i < end; ++i)
        raw = (raw << 8) | data[i];
    *value = ((float)raw + (float)raw_offset) * scale + (float)scaled_offset;
    return true;
}
