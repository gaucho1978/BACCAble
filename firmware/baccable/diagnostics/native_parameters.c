#include "app/powertrain.h"
#if defined(BACCABLE_C1)
extern char _end;
float native_parameter_read(uint8_t paramId) {

    switch (paramId) { // do preliminary additional stuff for special parameters (not uds)
    case 0:            // print oil pressure
        return (float)telemetry_state.oil_pressure * parameter_definitions[paramId].scale;
        break;
    case 1: // power in CV
        return (float)telemetry_state.torque * (float)telemetry_state.current_rpm_speed *
               parameter_definitions[paramId].scale;
        break;
    case 2: // torque in NM
        return (float)telemetry_state.torque;
        break;
    case 3: // battery state of charge (%)
        return (float)telemetry_state.battery_state_of_charge * parameter_definitions[paramId].scale;
        break;
    case 4: // battery current (A)
        return ((float)telemetry_state.battery_current * parameter_definitions[paramId].scale) +
               parameter_definitions[paramId].scaled_offset;
        break;
    case 5: // engine oil temperature
        return ((float)telemetry_state.oil_temperature * parameter_definitions[paramId].scale) +
               parameter_definitions[paramId].scaled_offset;
        break;
    case 6: // current gear
        return (float)(telemetry_state.current_gear >> 4);
        break;
    case 7: // current speed (km/h)
        return telemetry_state.current_speed_km_h;
        break;
    case 8: // DPF Regeneration type
        return (float)telemetry_state.diesel_engine_regeneration_mode;
        break;
    case 9:                                                          // stat 0-100km/h
        return statistics_state.chronometer_elapsed_time_0_100_km_h; // >=20=MISSED , increase in progress= GO
        break;
    case 10: // stat 100-200km/h
        return statistics_state
            .chronometer_elapsed_time_100_200_km_h; // >=40=MISSED , increase in progress= GO
        break;
    case 11: // Best stat 0-100km/h
        return statistics_read_best(1);
        break;
    case 12: // Best stat 100-200km/h
        return statistics_read_best(2);
        break;
    case 13: // seat belt alarm status
        return diagnostics_state.seatbelt_alarm_disabled;
        break;
    case 14: // debug param.
        break;
    case 15: // current Drive Style
        return telemetry_state.drive_mode;
        break;
    case 16: // free RAM
        return system_free_ram();
        break;
    case 17: // Pedal current Map
        return pedal_state.current_schizzaforte_map;
        break;
    default:
        break;
    }
    return 0;
}

uint32_t system_free_ram(void) {
    uint32_t stack_top;
    uint32_t heap_end = (uint32_t)&_end;

    // Ottieni il valore corrente dello stack pointer
    stack_top = (uint32_t)__get_MSP();

    // La RAM libera è lo spazio tra la fine dell'heap e lo stack corrente
    if (stack_top > heap_end) {
        return stack_top - heap_end;
    }
    return 0;
}

float accelerator_percent(uint8_t vol) {
    // Scale this value to get a percentage between 0 and 100
    // sniffed data on can bus: msg id=0x412 , fourth byte goes from 0x33(51) to 0xE6(230)
    if (vol > 50) {
        vol = vol - 51;
    }
    return (float)(((float)vol * 100.0f) / 180.0f);
}

uint8_t gear_color(uint8_t col) {
    // id 2ef, primo byte, 70=r, 00=n, f0=marcia inserita ma frizione premuta (indefinito), 10=prima,
    // 20=seconda..
    col = col >> 4;
    // status_led_blink_error(col);
    //  7=backward, f=gear set but frizione premuta (undefined), 1=first gear , 2=second gear, ... , 6= sixt
    //  gear
    return col;
}
#endif
