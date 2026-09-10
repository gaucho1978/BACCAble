#ifndef BACCABLE_STORAGE_RECORD_STORE_H
#define BACCABLE_STORAGE_RECORD_STORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define RECORD_MAX_PAYLOAD 128
#define RECORD_HEADER_SIZE 20
typedef struct {
    const uint8_t *pages[2];
    bool (*erase)(void *context, unsigned page);
    bool (*program)(void *context, unsigned page, size_t offset, uint16_t value);
    void *context;
} RecordStorage;
bool record_load(const RecordStorage *storage, uint16_t type, void *data, size_t length);
bool record_save(const RecordStorage *storage, uint16_t type, const void *data, size_t length);
#endif
