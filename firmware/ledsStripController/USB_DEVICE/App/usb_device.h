/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.h
  * @version        : v2.0_Cube
  * @brief          : Header for usb_device.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_DEVICE__H__
#define __USB_DEVICE__H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

	#include "stm32f0xx.h"
	#include "stm32f0xx_hal.h"



/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup USBD_OTG_DRIVER
  * @{
  */

/** @defgroup USBD_DEVICE USBD_DEVICE
  * @brief Device file for Usb otg low level driver.
  * @{
  */

/** @defgroup USBD_DEVICE_Exported_Variables USBD_DEVICE_Exported_Variables
  * @brief Public variables.
  * @{
  */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN VARIABLES */

/* USER CODE END VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_DEVICE_Exported_FunctionsPrototype USBD_DEVICE_Exported_FunctionsPrototype
  * @brief Declaration of public functions for Usb device.
  * @{
  */

/** USB Device initialization function. */
void MX_USB_DEVICE_Init(void);

//usb irq storm protection 22/11/2026 - BEGIN
//With no host on the connector some boards see spurious events on the usb lines and the usb interrupt (priority 0)
//fires back to back: SysTick (priority 3) and the main loop are starved for seconds, HAL_GetTick() stands still and
//the chip stops answering on the inter chip line. A real host can never cause this: even at full speed the usb
//interrupts are short and SysTick always gets through between two of them. So usb interrupts are counted in
//USB_IRQHandler and the count is cleared by every SysTick: reaching the threshold means SysTick could not run
//at all, and the usb is switched off for good, until the next reset (the usb cable is always connected before
//the baccable is powered, so a host that is really there is already there at power up).
#define USB_STORM_IRQ_THRESHOLD 254 //usb interrupts in a row with no SysTick in between (normal use: a few tens per ms at most)
extern volatile uint8_t usbStormIrqCount;		//usb interrupts since the last SysTick
extern volatile uint8_t usbDisabledUntilReboot;	//1 = a storm was detected: usb off, every usb bring up is skipped
extern volatile uint16_t usbStormIstr;			//USB->ISTR when the storm was detected (which event was looping), for diagnostics
//usb irq storm protection 22/11/2026 - END

//elm327 function 26/08/2026 - usb port brought up/down on request, from the "ELM327" entry of the dashboard
//setup menu: while the mode is off the connector stays silent. See usb_device.c.
#include "compile_time_defines.h"
#if defined(C1baccable)
	void usb_device_attach(void);
	void usb_device_detach(void);
#endif

/*
 * -- Insert functions declaration here --
 */
/* USER CODE BEGIN FD */

/* USER CODE END FD */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USB_DEVICE__H__ */
