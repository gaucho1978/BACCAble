/* Adapted from gaucho1978 BACCAble 02b2fd8; see LICENSE.MD and UPSTREAM_SYNC.md. */

#ifndef INC_ELM327_H_
#define INC_ELM327_H_
#include "app/build_config.h"
#ifdef ACT_AS_ELM327
    #include "stm32f0xx_hal.h"
    #include "transport/can_bus.h"
    #define ELM327_ID_STRING "ELM327 v1.4"
    #define ELM327_DESCR_STRING "OBDII to RS232 Interpreter"
    #define ELM327_STRICT_ELM_DEFAULTS
    #define ELM327_RX_RING_LEN 256
    #define ELM327_CMD_BUF_LEN 80
    #define ELM327_ISOTP_MAX_LEN 255
    #define ELM327_TX_CHUNK_LEN 60
    #define ELM327_PAD_BYTE 0xAA
    #define ELM327_MAX_RAW_FRAMES 64
    #define ELM327_ROUTE_CACHE_LEN 16
    #define ELM327_REMOTE_FIFO_LEN 12
    #define ELM327_PROBE_TIMEOUT_MS 200
    #define ELM327_DEFAULT_TIMEOUT_MS 200
    #define ELM327_FC_TIMEOUT_MS 250
    #define ELM327_TRACE_LEN 2048
void elm327_rx_lost(void);
void elm327_init(void);
void elm327_port_reset(void);
void elm327_rx_byte(uint8_t c);
void elm327_process(void);
    #if defined(BACCABLE_C1)
void elm327_set_enabled(uint8_t on);
uint8_t elm327_is_enabled(void);
        #ifndef ELM327_IDLE_EXIT_MS
            #define ELM327_IDLE_EXIT_MS 120000
        #endif
    #endif
#endif
#endif
