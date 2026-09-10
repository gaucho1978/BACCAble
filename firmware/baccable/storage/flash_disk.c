#include "app/build_config.h"
#include "storage/flash_layout.h"
#include "storage/flash_records.h"
#include "third_party/fatfs/ff.h"
#include "third_party/fatfs/diskio.h"
#include "stm32f0xx_hal.h"
#include <string.h>

static uint8_t page_buffer[STORAGE_PAGE_SIZE];

/* Report whether the device's file storage is available. */
DSTATUS disk_status(BYTE drive) { return drive == 0 && flash_storage_available() ? 0 : STA_NOINIT; }
/* Check that the device has the supported storage capacity. */
DSTATUS disk_initialize(BYTE drive) { return disk_status(drive); }

/* Read requested file-storage sectors within the reserved disk area. */
DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count) {
    if (drive || !buffer || !storage_sector_range(sector, count))
        return RES_PARERR;
    if (!flash_storage_available())
        return RES_NOTRDY;
    memcpy(buffer, (const void *)(USB_FLASH_START_ADDRESS + sector * STORAGE_SECTOR_SIZE),
           count * STORAGE_SECTOR_SIZE);
    return RES_OK;
}
#if FF_FS_READONLY == 0
/* Update file-storage sectors without overwriting neighboring data. */
DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count) {
    if (drive || !buffer || !storage_sector_range(sector, count))
        return RES_PARERR;
    if (!flash_storage_available() || __get_IPSR())
        return RES_NOTRDY;
    if (HAL_FLASH_Unlock() != HAL_OK)
        return RES_ERROR;
    DRESULT result = RES_OK;
    while (count) {
        uint32_t address = USB_FLASH_START_ADDRESS + sector * STORAGE_SECTOR_SIZE;
        uint32_t page = address - address % STORAGE_PAGE_SIZE;
        uint32_t offset = address - page;
        UINT sectors = (STORAGE_PAGE_SIZE - offset) / STORAGE_SECTOR_SIZE;
        if (sectors > count)
            sectors = count;
        memcpy(page_buffer, (const void *)page, sizeof(page_buffer));
        memcpy(page_buffer + offset, buffer, sectors * STORAGE_SECTOR_SIZE);
        if (memcmp(page_buffer, (const void *)page, sizeof(page_buffer))) {
            FLASH_EraseInitTypeDef erase = {
                .TypeErase = FLASH_TYPEERASE_PAGES, .PageAddress = page, .NbPages = 1};
            uint32_t error;
            if (HAL_FLASHEx_Erase(&erase, &error) != HAL_OK) {
                result = RES_ERROR;
                break;
            }
            for (unsigned i = 0; i < sizeof(page_buffer); i += 2) {
                uint16_t value = page_buffer[i] | (uint16_t)page_buffer[i + 1] << 8;
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, page + i, value) != HAL_OK) {
                    result = RES_ERROR;
                    break;
                }
            }
            if (result != RES_OK || memcmp(page_buffer, (const void *)page, sizeof(page_buffer))) {
                result = RES_ERROR;
                break;
            }
        }
        sector += sectors;
        count -= sectors;
        buffer += sectors * STORAGE_SECTOR_SIZE;
    }
    HAL_FLASH_Lock();
    return result;
}
#endif
/* Provide the storage geometry and completion status requested by the filesystem. */
DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer) {
    if (drive)
        return RES_PARERR;
    if (!flash_storage_available())
        return RES_NOTRDY;
    if (command == CTRL_SYNC)
        return RES_OK;
    if (!buffer)
        return RES_PARERR;
    switch (command) {
    case GET_SECTOR_COUNT:
        *(LBA_t *)buffer = STORAGE_SECTOR_COUNT;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buffer = STORAGE_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buffer = STORAGE_PAGE_SIZE / STORAGE_SECTOR_SIZE;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
