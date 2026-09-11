/* Adapted from gaucho1978 BACCAble 02b2fd8; see LICENSE.MD and UPSTREAM_SYNC.md. */

#ifndef INC_ELMLINK_H_
#define INC_ELMLINK_H_
#include "app/build_config.h"
#define ELMLINK_TO_C2 0x0E
#define ELMLINK_TO_BH 0x0F
#define ELMLINK_TO_MASTER 0x10
#define ELMLINK_TYPE_REQ 0x01
#define ELMLINK_TYPE_CFG 0x02
#define ELMLINK_TYPE_RSP 0x03
#define ELMLINK_TYPE_END 0x04
#define ELMLINK_TYPE_FCCFG 0x05
#define ELMLINK_TYPE_ARM 0x06
#define ELMLINK_FLAG_EXTID 0x01
#define ELMLINK_FLAG_NODATA 0x02
#define ELMLINK_FLAG_ARM_ON 0x01
#define ELMLINK_CFG_AUTOFC 0x01
#define ELMLINK_BUS_LOCAL 0
#define ELMLINK_BUS_C2 1
#define ELMLINK_BUS_BH 2
#define ELMLINK_BUS_COUNT 3
#define ELMLINK_DEFAULT_TIMEOUT_MS 300
#define ELMLINK_GAP_AFTER_SF_MS 40
#define ELMLINK_GAP_AFTER_FF_MS 250
#define ELMLINK_GAP_AFTER_CF_MS 80
#define ELMLINK_UART_TX_TIMEOUT_MS 30
#if defined(BACCABLE_C2)
    #define ELMLINK_SLAVE_ID ELMLINK_TO_C2
#elif defined(BACCABLE_BH)
    #define ELMLINK_SLAVE_ID ELMLINK_TO_BH
#endif
#if defined(BACCABLE_C1) || defined(BACCABLE_C2) || defined(BACCABLE_BH)
    #include "stm32f0xx_hal.h"
    #include "transport/can_bus.h"
void elmlink_init(void);
void elmlink_set_enabled(uint8_t on);
uint8_t elmlink_is_enabled(void);
uint8_t elmlink_on_uart_frame(const uint8_t *frame);
void elmlink_process(void);
    #if defined(BACCABLE_C2) || defined(BACCABLE_BH)
    #endif
    #if defined(BACCABLE_C1)
void elmlink_send_arm(uint8_t on);
uint8_t elmlink_send_config(uint8_t bus, uint32_t filterValue, uint32_t filterMask, uint16_t timeoutMs,
                            uint8_t autoFlowControl);
uint8_t elmlink_send_fc_config(uint8_t bus, uint32_t fcHeader, uint8_t fcExt, const uint8_t *fcData,
                               uint8_t fcLen);
uint8_t elmlink_send_request(uint8_t bus, uint32_t canId, uint8_t ext, const uint8_t *data, uint8_t dlc);
uint8_t elmlink_poll(void (*onFrame)(uint32_t id, uint8_t ext, const uint8_t *d, uint8_t dlc));
    #endif
#endif
#endif
