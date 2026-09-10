/**
 ******************************************************************************
 * @file           : usbd_storage_if.c
 * @version        : v2.0_Cube
 * @brief          : Memory management layer.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"
#include "app/application_state.h"
#include "storage/flash_records.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @brief Usb device.
 * @{
 */

/** @defgroup USBD_STORAGE
 * @brief Usb mass storage device module
 * @{
 */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
 * @brief Private types.
 * @{
 */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Defines
 * @brief Private defines.
 * @{
 */

#define STORAGE_LUN_NBR 1
#define STORAGE_BLK_NBR STORAGE_SECTOR_COUNT
#define STORAGE_BLK_SIZ 512

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Macros
 * @brief Private macros.
 * @{
 */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Variables
 * @brief Private variables.
 * @{
 */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_FS[] = {
    /* 36 */

    /* LUN 0 */
    0x00, 0x80, 0x02, 0x02, (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00, 0x00, 0x00, 'T',  'R',
    '3',  'M',  'A',  ' ',  ' ',
    ' ', /* Manufacturer : 8 bytes */
    'B',  'A',  'C',  'C',  'A',
    'b',  'l',  'e', /* Product      : 16 Bytes */
    ' ',  ' ',  ' ',  ' ',  ' ',
    ' ',  ' ',  ' ',  ' ',  ' ',
    ' ',  '1' /* Version      : 4 Bytes */
};

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Exported_Variables
 * @brief Public variables.
 * @{
 */

extern USBD_HandleTypeDef hUsbDeviceFS;

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
 * @brief Private functions declaration.
 * @{
 */

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

/**
 * @}
 */

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS = {
    STORAGE_Init_FS, STORAGE_GetCapacity_FS, STORAGE_IsReady_FS,   STORAGE_IsWriteProtected_FS,
    STORAGE_Read_FS, STORAGE_Write_FS,       STORAGE_GetMaxLun_FS, (int8_t *)STORAGE_Inquirydata_FS};

/* Private functions ---------------------------------------------------------*/
/* Notify the main board that a USB disk session has started on a companion board. */
int8_t STORAGE_Init_FS(uint8_t lun) {
// USB was plugged
// if BH or C2, send message to C1
#if (defined(BACCABLE_BH) || defined(BACCABLE_C2))
    status_led_activity();
    runtime_state.usb_connected_to_slave = 1; // we use this to avoid to unmount the disk
    uint8_t tmpArr2[2] = {C1BusID, C1usbConnected};
    board_uart_send(tmpArr2, 2);
    runtime_state.we_can_send_a_message_reply =
        TIMING__C2_BH_USB_CONNECT_TO_C1_NOTIFICATION_DELAY_MS; // enable sending the message thru serial line
                                                               // to C1, for a offset from now
#endif
    return (USBD_OK);
}

/* Report the size of the USB disk. */
int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size) {
    *block_num = STORAGE_BLK_NBR;
    *block_size = STORAGE_BLK_SIZ;
    return (USBD_OK);
}

/* Allow disk access only when the device has the required Flash capacity. */
int8_t STORAGE_IsReady_FS(uint8_t lun) { return lun == 0 && flash_storage_available() ? USBD_OK : USBD_FAIL; }

/* Present the USB disk as read-only to the host. */
int8_t STORAGE_IsWriteProtected_FS(uint8_t lun) { return 1; }

/* Read a valid range of sectors from the device USB disk. */
int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    if (lun || !buf || !flash_storage_available() || !storage_sector_range(blk_addr, blk_len))
        return USBD_FAIL;
#ifdef ENABLE_USB_MASS_STORAGE
    memcpy(buf, (const void *)(USB_FLASH_START_ADDRESS + (blk_addr * STORAGE_BLK_SIZ)),
           (blk_len * STORAGE_BLK_SIZ));
#endif
    return (USBD_OK);
}

/* Reject host writes to protect the device USB disk. */
int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
    return (USBD_FAIL);
}

/* Report the single disk exposed by this device. */
int8_t STORAGE_GetMaxLun_FS(void) { return (STORAGE_LUN_NBR - 1); }

/**
 * @}
 */

/**
 * @}
 */
