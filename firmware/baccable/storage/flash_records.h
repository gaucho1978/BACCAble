#ifndef BACCABLE_STORAGE_FLASH_RECORDS_H
#define BACCABLE_STORAGE_FLASH_RECORDS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { SETTINGS_RECORD, STATISTICS_RECORD, VISIBILITY_RECORD };
bool flash_storage_available(void);
bool flash_record_load(unsigned record, uint16_t type, void *data, size_t length);
bool flash_record_save(unsigned record, uint16_t type, const void *data, size_t length);
#endif
