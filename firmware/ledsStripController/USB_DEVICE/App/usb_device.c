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
#include "compile_time_defines.h"

#include "usbd_core.h"
#include "usbd_desc.h"

#ifdef ENABLE_USB_MASS_STORAGE
	#include "usbd_msc.h"
	#include "usbd_storage_if.h"
#else
	#include "usbd_cdc.h"
	#include "usbd_cdc_if.h"
#endif

#include "main.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void){

	// --- Reset USB core --- prevents problems when exiting from a Hardware Reset
	__HAL_RCC_USB_FORCE_RESET();
	HAL_Delay(2);
	__HAL_RCC_USB_RELEASE_RESET();
	HAL_Delay(10);

	/* Init Device Library, add supported class and start the library. */

	///*commented for test in V.3.0.0a
	if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK){
		//onboardLed_red_on();
		//onboardLed_blue_on();
		Error_Handler(6000);
	}

	//*/

	///*commented for test in V.3.0.0a and V.3.0.0b
	#ifdef ENABLE_USB_MASS_STORAGE
		if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC) != USBD_OK){
	#else
		if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK){
	#endif
			//onboardLed_red_on();
			//onboardLed_blue_on();
			Error_Handler(6500);
		}
	//*/

	///*commented for test in V.3.0.0a, V.3.0.0b and V.3.0.0c
	#ifdef ENABLE_USB_MASS_STORAGE
		if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK){
	#else
		if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK){
	#endif
			//onboardLed_red_on();
			//onboardLed_blue_on();
			Error_Handler(7000);
		}

	if (USBD_Start(&hUsbDeviceFS) != USBD_OK){
		//onboardLed_red_on();
		//onboardLed_blue_on();
		Error_Handler(7500);
	}

	//*/
}
