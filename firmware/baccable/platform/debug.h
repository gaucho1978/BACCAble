#ifndef BACCABLE_PLATFORM_DEBUG_H
#define BACCABLE_PLATFORM_DEBUG_H
#include "app/build_config.h"

#ifdef DEBUG_MODE
    #include "usbd_cdc_if.h"
    #pragma message("Enabling debug over USB for this build")
    #define LOGS(message) print_to_usb_(message)                   // simple string
    #define LOG(format, ...) printf_to_usb_(format, ##__VA_ARGS__) // formatted as sprintf
#else
    #define LOG(...)      // noop
    #define LOGS(message) // noop
#endif

#endif /* BACCABLE_PLATFORM_DEBUG_H */
