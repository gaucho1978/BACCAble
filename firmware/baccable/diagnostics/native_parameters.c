#include "app/powertrain.h"
#if defined(BACCABLE_C1)
extern char _end;

/* Read a display value already available from vehicle reports or local records. */
float native_parameter_read(uint8_t paramId) {

    switch (paramId) { // do preliminary additional stuff for special parameters (not uds)
    case 0:            // print oil pressure
        return (float)telemetry_state.oil_pressure * parameter_definitions[paramId].scale;
    case 1: // power in CV
        return (float)telemetry_state.torque * (float)telemetry_state.current_rpm_speed *
               parameter_definitions[paramId].scale;
    case 2: // torque in NM
        return (float)telemetry_state.torque;
    case 3: // battery state of charge (%)
        return (float)telemetry_state.battery_state_of_charge * parameter_definitions[paramId].scale;
    case 4: // battery current (A)
        return ((float)telemetry_state.battery_current * parameter_definitions[paramId].scale) +
               parameter_definitions[paramId].scaled_offset;
    case 5: // engine oil temperature
        return ((float)telemetry_state.oil_temperature * parameter_definitions[paramId].scale) +
               parameter_definitions[paramId].scaled_offset;
    case 6: // current gear
        return (float)(telemetry_state.current_gear >> 4);
    case 7: // current speed (km/h)
        return telemetry_state.current_speed_km_h;
    case 8: // DPF Regeneration type
        return (float)telemetry_state.diesel_engine_regeneration_mode;
    case 9:                                                          // stat 0-100km/h
        return statistics_state.chronometer_elapsed_time_0_100_km_h; // >=20=MISSED , increase in progress= GO
    case 10:                                                         // stat 100-200km/h
        return statistics_state
            .chronometer_elapsed_time_100_200_km_h; // >=40=MISSED , increase in progress= GO
    case 11:                                        // Best stat 0-100km/h
        return statistics_read_best(1);
    case 12: // Best stat 100-200km/h
        return statistics_read_best(2);
    case 13: // seat belt alarm status
        return diagnostics_state.seatbelt_alarm_disabled;
    case 14: // debug param.
        break;
    case 15: // current Drive Style
        return telemetry_state.drive_mode;
    case 16: // free RAM
        return system_free_ram();
    case 17: // Pedal current Map
        return pedal_state.current_schizzaforte_map;
    default:
        break;
    }
    return 0;
}

/* Estimate the memory headroom available to the running device. */
uint32_t system_free_ram(void) {
    uint32_t stack_top;
    uint32_t heap_end = (uint32_t)&_end;

    // Read the current stack boundary.
    stack_top = (uint32_t)__get_MSP();

    // Report the gap between the reserved memory area and the current stack.
    if (stack_top > heap_end) {
        return stack_top - heap_end;
    }
    return 0;
}

/* Express the reported accelerator position as a percentage. */
float accelerator_percent(uint8_t vol) {
    // Scale this value to get a percentage between 0 and 100
    // sniffed data on can bus: msg id=0x412 , fourth byte goes from 0x33(51) to 0xE6(230)
    if (vol > 50) {
        vol = vol - 51;
    }
    return (float)(((float)vol * 100.0f) / 180.0f);
}

/* Choose the LED color preset associated with the reported gear. */
uint8_t gear_color(uint8_t col) {
    // Gear byte: 0x70 reverse, 0x00 neutral, 0xF0 unavailable, 0x10 first gear.
    // 0x20 denotes second gear.
    col = col >> 4;

    //  7 is reverse, F is unavailable, and 1 through 6 select the corresponding gear.

    return col;
}
#endif
