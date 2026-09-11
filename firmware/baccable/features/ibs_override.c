/* Optional IBS experiment adapted from netzmark BACCAble a3ca082. */
#include "features/ibs_override.h"
#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
static bool enabled;
static uint8_t pending, replacement[7];
static uint32_t observed, sent;

/* Explicitly enable the temporary SOC override; it is never saved or activated by browsing readings. */
void ibs_override_enable(bool on) {
    enabled = on;
    pending = 0;
}

/* Report whether the temporary charging experiment is enabled. */
bool ibs_override_enabled(void) { return enabled; }

/* Prepare an override only from a complete, recent IBS report in the upstream SOC range. */
void ibs_override_observe(const CAN_RxHeaderTypeDef *h, const uint8_t *data) {
    if (!enabled || h->IDE != CAN_ID_STD || h->RTR != CAN_RTR_DATA || h->StdId != 0x41a || h->DLC < 7)
        return;
    unsigned soc = data[1] & 0x7f;
    pending = 0;
    if (soc >= 75 && soc < 98) {
        memcpy(replacement, data, sizeof(replacement));
        replacement[1] = (data[1] & 0x80) | 75;
        observed = currentTime;
        pending = 10;
    }
}

/* Repeat the upstream SOC override briefly, stopping on stale input or engine shutdown. */
void ibs_override_process(void) {
    if (telemetry_state.current_rpm_speed <= 400)
        ibs_override_enable(false);
    if (!enabled || !pending)
        return;
    if (currentTime - observed > 100) {
        pending = 0;
        return;
    }
    CAN_TxHeaderTypeDef header = {.StdId = 0x41a, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 7};
    if (currentTime - sent >= 3 && can_tx(&header, replacement) == HAL_OK) {
        --pending;
        sent = currentTime;
    }
}
#endif
