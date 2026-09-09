#include "storage/flash_records.h"
#include "storage/flash_layout.h"
#include "storage/record_store.h"
#include "stm32f0xx_hal.h"

bool flash_storage_available(void) { return *(const volatile uint16_t *)FLASHSIZE_BASE >= 128; }
static bool erase_page(void *context, unsigned page) {
    uint32_t base = *(uint32_t *)context;
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES, .PageAddress = base + page * STORAGE_PAGE_SIZE, .NbPages = 1};
    uint32_t error;
    return HAL_FLASHEx_Erase(&erase, &error) == HAL_OK;
}
static bool program_word(void *context, unsigned page, size_t offset, uint16_t value) {
    uint32_t base = *(uint32_t *)context;
    return HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, base + page * STORAGE_PAGE_SIZE + offset, value) ==
           HAL_OK;
}
static RecordStorage storage_at(uint32_t *base) {
    RecordStorage storage = {.pages = {(const uint8_t *)*base, (const uint8_t *)(*base + STORAGE_PAGE_SIZE)},
                             .erase = erase_page,
                             .program = program_word,
                             .context = base};
    return storage;
}
bool flash_record_load(unsigned record, uint16_t type, void *data, size_t length) {
    if (record > VISIBILITY_RECORD || !flash_storage_available())
        return false;
    uint32_t base = STORAGE_RECORDS_START + record * 2 * STORAGE_PAGE_SIZE;
    RecordStorage storage = storage_at(&base);
    return record_load(&storage, type, data, length);
}
bool flash_record_save(unsigned record, uint16_t type, const void *data, size_t length) {
    if (record > VISIBILITY_RECORD || !flash_storage_available() || __get_IPSR())
        return false;
    uint32_t base = STORAGE_RECORDS_START + record * 2 * STORAGE_PAGE_SIZE;
    RecordStorage storage = storage_at(&base);
    if (HAL_FLASH_Unlock() != HAL_OK)
        return false;
    bool result = record_save(&storage, type, data, length);
    HAL_FLASH_Lock();
    return result;
}
