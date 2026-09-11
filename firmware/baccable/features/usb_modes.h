#ifndef BACCABLE_USB_MODES_H
#define BACCABLE_USB_MODES_H
#include "app/build_config.h"
#include "stm32f0xx_hal.h"
#include <stdbool.h>
void usb_modes_apply(void);
void usb_modes_process(void);
void usb_modes_set_sniffer(bool enabled);
void usb_modes_peer(uint8_t peer, bool connected);
bool usb_modes_active(void);
void usb_sniffer_observe(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
#endif
