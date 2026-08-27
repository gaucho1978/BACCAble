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
	#include "usbd_cdc.h" //sniffer function 24/08/2026 - sniffer switches these builds to cdc at runtime
	#include "usbd_cdc_if.h" //sniffer function 24/08/2026
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

	//sniffer function 24/08/2026 - BEGIN
	//on the mass storage builds (C2 and BH) the sniffer gives up the pen drive and comes back as cdc.
	//the mass storage path below is left untouched and is still the one taken when the sniffer is off.
	#ifdef ENABLE_USB_MASS_STORAGE
		if(usbdDescCdcModeSelected){
			if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK){
				Error_Handler(6500);
			}
			if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK){
				Error_Handler(7000);
			}
			if (USBD_Start(&hUsbDeviceFS) != USBD_OK){
				Error_Handler(7500);
			}
			return;
		}
	#endif
	//sniffer function 24/08/2026 - END

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

//elm327 function 26/08/2026 - BEGIN
#if defined(C1baccable)
	// USB PORT BROUGHT UP AND TAKEN DOWN ON REQUEST (ELM327 mode chosen from the dashboard menu).
	//
	// While the mode is off the connector must stay silent: plugging the baccable into a computer must not
	// make any diagnostic interface appear.
	//
	// Note why everything is redone here instead of using USBD_Start/USBD_Stop: USBD_Stop calls
	// USB_StopDevice(), which leaves the CNTR register at (FRES | PDWN), i.e. the usb block in reset and
	// powered down, and USBD_Start only ORs the interrupt bits back in without clearing PDWN. After one
	// off/on cycle the port would never come back. A peripheral reset and a full init are required.
	//
	// Error_Handler() is never called here: it is a while(1) and would freeze the car. If anything fails the
	// port simply does not come up and the baccable keeps working.
	static uint8_t usbAttached = 0;

	void usb_device_attach(void){
		if(usbAttached) return;

		usbdDescSelectElm327Mode(); //descriptors must answer as the elm327 emulator before the host enumerates us

		__HAL_RCC_USB_FORCE_RESET();
		HAL_Delay(2);
		__HAL_RCC_USB_RELEASE_RESET();
		HAL_Delay(10);

		if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) return;
		if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) return;
		if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) return;
		if (USBD_Start(&hUsbDeviceFS) != USBD_OK) return;

		usbAttached = 1;
	}

	void usb_device_detach(void){
		if(!usbAttached) return;
		usbAttached = 0;

		//release the D+ pull-up first and hold it low long enough for the host to register the removal:
		//the teardown below resets and powers the core down but never clears BCDR_DPPU on its own, so
		//without this the host would just see the device go quiet rather than actually detach.
		if(hUsbDeviceFS.pData!=NULL){
			HAL_PCD_DevDisconnect((PCD_HandleTypeDef*)hUsbDeviceFS.pData);
			HAL_Delay(250);
		}
		//pClassData is only allocated once the host has sent SET_CONFIGURATION: with no host it is still NULL,
		//and USBD_DeInit() would call pClass->DeInit() on it without any NULL check.
		if(hUsbDeviceFS.pClassData!=NULL){
			USBD_DeInit(&hUsbDeviceFS);
		}else{
			USBD_LL_Stop(&hUsbDeviceFS);
			USBD_LL_DeInit(&hUsbDeviceFS);
		}
	}
#endif
//elm327 function 26/08/2026 - END
