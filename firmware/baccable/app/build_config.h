#ifndef BACCABLE_APP_BUILD_CONFIG_H
#define BACCABLE_APP_BUILD_CONFIG_H

#ifdef INCLUDE_USER_CONFIG_H
    #include "user_config.h"
#endif

#ifndef BUILD_VERSION
    #define BUILD_VERSION GIT_VERSION
#endif
#define _FW_VERSION "BACCABLE " BUILD_VERSION

/* The Makefile selects exactly one flavor. A standalone host build defaults to CAN. */
#if (defined(C1_FLAVOR) + defined(C2_FLAVOR) + defined(BH_FLAVOR) + defined(CAN_FLAVOR)) > 1
    #error "Select exactly one firmware flavor"
#endif
#if defined(C1_FLAVOR)
    #define BACCABLE_C1
#elif defined(C2_FLAVOR)
    #define BACCABLE_C2
#elif defined(BH_FLAVOR)
    #define BACCABLE_BH
#else
    #define ACT_AS_CANABLE
#endif

#ifdef LARGE_DISPLAY
    #define DASHBOARD_MESSAGE_MAX_LENGTH 24
#else
    #define DASHBOARD_MESSAGE_MAX_LENGTH 18
#endif

/* The dedicated BACCAble and UCAN boards use active-low status LEDs. */
#ifndef DISABLE_UCAN_BOARD_LED_INVERSION
    #define UCAN_BOARD_LED_INVERSION
#endif

/* C1: powertrain bus, OBD pins 6 and 14. Other feature defaults live in state/. */
#ifdef BACCABLE_C1
    #ifndef DISABLE_CLEAR_FAULTS_ENABLED
        #define CLEAR_FAULTS_ENABLED
    #endif
    #ifndef DISABLE_LOW_CONSUME
        #define LOW_CONSUME
    #endif
    #ifndef IS_GASOLINE
        #define IS_DIESEL
    #endif
    #ifdef PERMANENTLY_DISABLE_IMMO
        #define DISABLE_IMMOBILIZER
    #endif
    #ifndef DISABLE_IMMOBILIZER
        #define IMMOBILIZER_ENABLED
    #endif
    #ifndef DISABLE_THE_FUNCTION_SMART_DISABLE_START_STOP
        #define SMART_DISABLE_START_STOP
    #endif
    #ifndef SHIFT_THRESHOLD
        #define SHIFT_THRESHOLD 4500
    #endif
    #ifndef DISABLE_DPF_REGEN_VISUAL_ALERT
        #define DPF_REGEN_VISUAL_ALERT
    #endif
    #ifndef DISABLE_DPF_REGEN_SOUND_ALERT
        #define DPF_REGEN_SOUND_ALERT
    #endif
#endif

/* C2: chassis bus, OBD pins 12 and 13. */
#ifdef BACCABLE_C2
    #ifndef DISABLE_ESC_TC_CUSTOMIZATOR
        #define ESC_TC_CUSTOMIZATOR_ENABLED
    #endif
    #ifndef DISABLE_DYNO_MODE
        #define DYNO_MODE
    #endif
    #ifndef DISABLE_FRONT_BRAKE_FORCER
        #define FRONT_BRAKE_FORCER
    #endif
    #ifndef DISABLE_CLEAR_FAULTS_C2
        #define CLEAR_FAULTS_ENABLED
    #endif
#endif

/* BH: body bus, OBD pins 3 and 11. */
#ifdef BACCABLE_BH
    #ifndef DISPLAY_INFO_CODE
        #define DISPLAY_INFO_CODE 0x09
    #endif
    #ifndef DISABLE_CLEAR_FAULTS_BH
        #define CLEAR_FAULTS_ENABLED
    #endif
#endif

#if defined(DEBUG_MODE) && (defined(ACT_AS_CANABLE) || defined(LED_STRIP_CONTROLLER_ENABLED))
    #error "DEBUG_MODE conflicts with CANable and LED_STRIP_CONTROLLER_ENABLED"
#endif
#if (defined(ACT_AS_CANABLE) + defined(BACCABLE_C1) + defined(BACCABLE_C2) + defined(BACCABLE_BH)) != 1
    #error "CANable and vehicle board roles are mutually exclusive"
#endif
#if defined(ACT_AS_SCHIZZAFORTE_SERIAL_CONTROLLER) && !defined(BACCABLE_C1)
    #error "The pedal serial adapter requires the C1 UART hardware"
#endif
#if defined(SMART_DISABLE_START_STOP) && defined(DISABLE_START_STOP)
    #error "Choose SMART_DISABLE_START_STOP or DISABLE_START_STOP"
#endif

#if !defined(DEBUG_MODE) && (defined(BACCABLE_C2) || defined(BACCABLE_BH))
    #define ENABLE_USB_MASS_STORAGE
#endif

#include "storage/flash_layout.h"
#endif
