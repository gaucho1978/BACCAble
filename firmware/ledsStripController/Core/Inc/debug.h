/*
 * debug.h
 *
 *  Created on: May 13, 2025
 *      Author: alessandro
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_
#include "compile_time_defines.h"

#ifdef DEBUG_MODE
#include "usbd_cdc_if.h"
#warning "enabling debug over USB for this build"
#define LOGS(message) print_to_usb_(message)//simple string
#define LOG(format, ...) printf_to_usb_(format, ##__VA_ARGS__)//formatted as sprintf
#else
#define LOG(...)//noop
#define LOGS(message)//noop
#endif

#endif /* INC_DEBUG_H_ */
