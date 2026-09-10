#include "diagnostics/parameter_cache.h"
#include "app/build_config.h"
#if defined(BACCABLE_C1)
    #include "app/powertrain.h"
    #include <string.h>
static float values[100];
static uint32_t updated[100];
static uint8_t valid[(100 + 7) / 8];
static uint32_t rpm_updated;
static uint8_t rpm_valid;

/* Forget previous measurements when starting or changing the engine profile. */
void parameter_cache_reset(void) {
    memset(valid, 0, sizeof(valid));
    rpm_valid = 0;
}

/* Remember a fresh measurement and invalidate unavailable values. */
void parameter_cache_put(uint8_t id, float value, uint32_t now) {
    if (id >= 100)
        return;
    values[id] = value;
    updated[id] = now;
    uint8_t mask = 1U << (id % 8);
    if (isfinite(value))
        valid[id / 8] |= mask;
    else
        valid[id / 8] &= (uint8_t)~mask;
}

/* Return the latest usable reading, or mark it unavailable when it is stale. */
float parameter_cache_get(uint8_t id, uint32_t now) {
    if (id == 11 || id == 12 || id == 16)
        return native_parameter_read(id);
    if (id >= 100 || !(valid[id / 8] & (1U << (id % 8))) || now - updated[id] > 3000)
        return NAN;
    return values[id];
}

/* Refresh a reading from the latest reported vehicle state. */
static void native_update(uint8_t id) { parameter_cache_put(id, native_parameter_read(id), currentTime); }

/* Refresh only the measurements supplied by the incoming vehicle update. */
void parameter_cache_observe(uint32_t id, uint8_t length) {
    switch (id) {
    case 0xfc:
        if (length >= 2) {
            rpm_updated = currentTime;
            rpm_valid = 1;
        }
        break;
    case 0xfb:
        if (length >= 4) {
            native_update(2);
            if (rpm_valid && currentTime - rpm_updated <= 1000)
                native_update(1);
        }
        break;
    case 0x101:
        if (length >= 3) {
            native_update(7);
            native_update(9);
            native_update(10);
        }
        break;
    case 0x2ef:
        if (length >= 1)
            native_update(6);
        break;
    case 0x41a:
        if (length >= 6) {
            native_update(3);
            native_update(4);
        }
        break;
    case 0x4b2:
        if (length >= 4) {
            native_update(0);
            native_update(5);
        }
        break;
    case 0x5ae:
        if (length >= 6)
            native_update(8);
        break;
    case 0x384:
        if (length >= 8)
            native_update(15);
        break;
    default:
        break;
    }
}
#endif
