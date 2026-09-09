#include "storage/record_store.h"
#include <string.h>

static uint16_t read16(const uint8_t *data) { return (uint16_t)data[0] | (uint16_t)data[1] << 8; }
static uint32_t read32(const uint8_t *data) {
    return (uint32_t)read16(data) | (uint32_t)read16(data + 2) << 16;
}
static void write16(uint8_t *data, uint16_t value) {
    data[0] = value;
    data[1] = value >> 8;
}
static void write32(uint8_t *data, uint32_t value) {
    write16(data, value);
    write16(data + 2, value >> 16);
}
static uint32_t crc_add(uint32_t crc, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
    }
    return crc;
}
static uint32_t record_crc(const uint8_t *record, size_t length) {
    return ~crc_add(crc_add(0xffffffffU, record, 12), record + RECORD_HEADER_SIZE, length);
}
static bool valid(const uint8_t *page, uint16_t type, size_t length) {
    return page && read32(page) == 0x42414343U && read16(page + 4) == type && read16(page + 6) == length &&
           read16(page + 16) == 0 && read16(page + 18) == 1 && read32(page + 12) == record_crc(page, length);
}
static int latest(const RecordStorage *storage, uint16_t type, size_t length) {
    bool first = valid(storage->pages[0], type, length);
    bool second = valid(storage->pages[1], type, length);
    if (!first)
        return second ? 1 : -1;
    if (!second)
        return 0;
    uint32_t difference = read32(storage->pages[1] + 8) - read32(storage->pages[0] + 8);
    return difference != 0 && difference < 0x80000000U ? 1 : 0;
}
bool record_load(const RecordStorage *storage, uint16_t type, void *data, size_t length) {
    if (!storage || !data || !length || length > RECORD_MAX_PAYLOAD)
        return false;
    int page = latest(storage, type, length);
    if (page < 0)
        return false;
    memcpy(data, storage->pages[page] + RECORD_HEADER_SIZE, length);
    return true;
}
bool record_save(const RecordStorage *storage, uint16_t type, const void *data, size_t length) {
    if (!storage || !storage->erase || !storage->program || !data || !length || length > RECORD_MAX_PAYLOAD)
        return false;
    int previous = latest(storage, type, length);
    if (previous >= 0 && !memcmp(storage->pages[previous] + RECORD_HEADER_SIZE, data, length))
        return true;
    unsigned next = previous == 0 ? 1 : 0;
    uint32_t generation = previous < 0 ? 0 : read32(storage->pages[previous] + 8) + 1;
    uint8_t record[RECORD_HEADER_SIZE + RECORD_MAX_PAYLOAD];
    memset(record, 0xff, sizeof(record));
    write32(record, 0x42414343U);
    write16(record + 4, type);
    write16(record + 6, length);
    write32(record + 8, generation);
    write16(record + 18, 1);
    memcpy(record + RECORD_HEADER_SIZE, data, length);
    write32(record + 12, record_crc(record, length));
    if (!storage->erase(storage->context, next))
        return false;
    size_t end = RECORD_HEADER_SIZE + ((length + 1) & ~(size_t)1);
    for (size_t offset = 0; offset < end; offset += 2) {
        if (offset == 16)
            continue;
        if (!storage->program(storage->context, next, offset, read16(record + offset)))
            return false;
    }
    /* The previous page remains valid until this last halfword is committed. */
    if (!storage->program(storage->context, next, 16, 0))
        return false;
    return valid(storage->pages[next], type, length);
}
