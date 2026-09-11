/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usb_device.c
 * @version        : v2.0_Cube
 * @brief          : This file implements the USB Device
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "usb_device.h"
#include "app/build_config.h"

#include "usbd_core.h"
#include "usbd_desc.h"

#include "usbd_msc.h"
#include "usbd_storage_if.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#include "app/main.h"

USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t started, serial_mode;

/* Report the active USB class before class-specific data is accessed. */
uint8_t usb_device_is_serial(void) { return serial_mode; }

/* Disconnect the active USB function and release its class state. */
void usb_device_stop(void) {
    if (!started)
        return;
    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);
    started = 0;
}

/* Start the selected USB function, with a fresh host enumeration after mode changes. */
void usb_device_start(uint8_t serial) {
    if (started && serial_mode == !!serial)
        return;
    usb_device_stop();
    serial_mode = !!serial;
    __HAL_RCC_USB_FORCE_RESET();
    HAL_Delay(2);
    __HAL_RCC_USB_RELEASE_RESET();
    HAL_Delay(20);
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
        Error_Handler(6000);
    if (USBD_RegisterClass(&hUsbDeviceFS, serial ? &USBD_CDC : &USBD_MSC) != USBD_OK)
        Error_Handler(6500);
    uint8_t result = serial ? USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS)
                            : USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS);
    if (result != USBD_OK)
        Error_Handler(7000);
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
        Error_Handler(7500);
    started = 1;
}

/* Start the board's normal USB disk or serial interface. */
void MX_USB_DEVICE_Init(void) {
#ifdef ENABLE_USB_MASS_STORAGE
    usb_device_start(0);
#else
    usb_device_start(1);
#endif
}
