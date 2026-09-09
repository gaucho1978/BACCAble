#include "app/powertrain.h"
#include <stdbool.h>
#if defined(BACCABLE_C1)
static void append_text(char *output, size_t *length, const char *text) {
    while (*text && *length < DASHBOARD_MESSAGE_MAX_LENGTH)
        output[(*length)++] = *text++;
}

static unsigned enum_index(float value, unsigned fallback) {
    return isfinite(value) && value >= 0 && value < fallback ? (unsigned)value : fallback;
}

static const char *enum_text(uint32_t id, float value, char symbol[2]) {
    switch (id) {
    case 0x17:
        symbol[0] = value >= 0 && value < 11 ? gear_symbols[(unsigned)value] : '-';
        return symbol;
    case 0x19:
        return regeneration_labels[enum_index(value, 7)];
    case 0x1e:
        return seatbelt_labels[enum_index(value, 2)];
    case 0x20:
        switch (telemetry_state.drive_mode) {
        case 0x00:
            return "N";
        case 0x08:
            return "D";
        case 0x10:
            return "A";
        case 0x30:
            return "R";
        default:
            return "?";
        }
    case 0x22:
        symbol[0] = pedal_state.current_schizzaforte_map;
        return symbol;
    default:
        return "";
    }
}

static const char *number_text(uint32_t id, float value, unsigned decimals, unsigned width, char buffer[20]) {
    bool short_run = id == 0x1a || id == 0x1c;
    bool long_run = id == 0x1b || id == 0x1d;
    if ((short_run && value > 20) || (long_run && value > 40))
        return statistics_labels[0];
    if ((id == 0x1a && statistics_state.statistics_0_100_started) ||
        (id == 0x1b && statistics_state.statistics_100_200_started))
        return statistics_labels[1];
    format_number(buffer, value, decimals, width + 1);
    return buffer;
}

static bool number_placeholder(const char *text) {
    return strlen(text) >= 5 && text[0] == '$' && text[1] >= '0' && text[1] <= '9' && text[2] == '.' &&
           text[3] >= '0' && text[3] <= '9' && text[4] == 'f';
}

/* The caller provides DASHBOARD_MESSAGE_MAX_LENGTH + 1 output bytes. */
void dashboard_format_values(const char *template, float values[2], const uint8_t paramId[2], char *result) {
    size_t length = 0;
    unsigned element = 0;
    for (const char *text = template; *text && length < DASHBOARD_MESSAGE_MAX_LENGTH;) {
        bool numeric = number_placeholder(text);
        bool enumeration = !strncmp(text, "$enum", 5);
        if (element >= 2 || paramId[element] >= 100 || (!numeric && !enumeration)) {
            result[length++] = *text++;
            continue;
        }
        uint32_t id = parameter_definitions[paramId[element]].request_id;
        char buffer[20];
        char symbol[2] = {0};
        const char *formatted;
        if (numeric) {
            unsigned decimals = text[3] - '0';
            unsigned width = text[1] - '0' + decimals + (decimals > 0);
            formatted = number_text(id, values[element], decimals, width, buffer);
        } else {
            formatted = enum_text(id, values[element], symbol);
        }
        append_text(result, &length, formatted);
        ++element;
        text += 5;
    }
    result[length] = '\0';
}

uint8_t dashboard_remove_placeholders(char *text) {
    char *read = text, *write = text;
    while (*read) {
        if (number_placeholder(read) || !strncmp(read, "$enum", 5)) {
            read += 5;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
    return (uint8_t)(write - text);
}
#endif
