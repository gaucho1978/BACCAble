#include "platform/system.h"
/* Provide the integrity check expected by supported vehicle messages. */
uint8_t frame_checksum(uint8_t *data, uint8_t arraySize) {
    uint8_t crc = 0xFF;
    if (arraySize > 1) {
        // calculate sae_j1850 CRC-8 of the array (excluded last element, that will be used to store the final
        // CRC
        for (uint8_t i = 0; i < arraySize - 1; i++) {
            crc ^= data[i];
            for (int i = 0; i < 8; ++i) {
                crc = (crc & 0x80) ? (crc << 1) ^ 0x1D : crc << 1;
            }
        }
        return (crc ^ 0xFF);
    }
    return 0; // nothing to calculate
}
