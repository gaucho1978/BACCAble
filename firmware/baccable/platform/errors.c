
// Error: handling / reporting of system errors

#include "stm32f0xx_hal.h"
#include "platform/errors.h"

static uint32_t err_reg = 0;

/* Remember a communication fault for host status reporting. */
void error_assert(error_t err) {
    if (err >= ERR_MAX)
        return;

    err_reg |= (1 << err);
}

/* Report the communication faults observed since startup. */
uint32_t error_reg(void) { return err_reg; }
