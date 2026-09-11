#ifndef TEST_USB_DEVICE_H
#define TEST_USB_DEVICE_H
#include "usbd_cdc.h"
void usb_device_start(uint8_t serial);
void usb_device_stop(void);
uint8_t usb_device_is_serial(void);
void MX_USB_DEVICE_Init(void);
#endif
