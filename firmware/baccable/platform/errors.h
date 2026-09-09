#ifndef BACCABLE_PLATFORM_ERRORS_H
#define BACCABLE_PLATFORM_ERRORS_H
#include <stdint.h>

typedef enum _error_t {
    ERR_PERIPHINIT = 0,
    ERR_USBTX_BUSY,
    ERR_CAN_TXFAIL,
    ERR_CANRXFIFO_OVERFLOW,
    ERR_FULLBUF_CANTX,
    ERR_FULLBUF_USBRX,

    ERR_MAX
} error_t;

// Prototypes
void error_assert(error_t err);
uint32_t error_timestamp(error_t err);
uint8_t error_occurred(error_t err);
uint32_t error_reg(void);

#endif /* BACCABLE_PLATFORM_ERRORS_H */
