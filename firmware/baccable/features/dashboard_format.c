#include "app/powertrain.h"
#include <stdbool.h>
#if defined(BACCABLE_C1)

/* Append display text while respecting the available screen width. */
static void append_text(char *output, size_t *length, const char *text) {
    while (*text && *length < DASHBOARD_MESSAGE_MAX_LENGTH)
        output[(*length)++] = *text++;
}

/* Choose a known status label or the unavailable fallback. */
static unsigned enum_index(float value, unsigned fallback) {
    return isfinite(value) && value >= 0 && value < fallback ? (unsigned)value : fallback;
}

/* Describe a reported gear, regeneration, drive mode or pedal-map status. */
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
        switch ((unsigned)value) {
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
        symbol[0] = (char)value;
        return symbol;
    default:
        return "";
    }
}

/* Format a reading or performance-run status without showing truncated numbers. */
static const char *number_text(uint32_t id, float value, unsigned decimals, unsigned width, char buffer[20]) {
    bool short_run = id == 0x1a || id == 0x1c;
    bool long_run = id == 0x1b || id == 0x1d;
    if ((short_run && value > 20) || (long_run && value > 40))
        return statistics_labels[0];
    if ((id == 0x1a && statistics_state.statistics_0_100_started) ||
        (id == 0x1b && statistics_state.statistics_100_200_started))
        return statistics_labels[1];
    if (width > 19)
        width = 19;
    char number[32];
    float half_step = 0.5f;
    for (unsigned i = 0; i < decimals; ++i)
        half_step *= 0.1f;
    if (value < 0 && value > -half_step)
        value = 0;
    int length =
        isfinite(value) ? snprintf_(number, sizeof(number), "%.*f", (int)decimals, (double)value) : -1;
    memset(buffer, ' ', width);
    buffer[width] = 0;
    if (length < 0 || (unsigned)length > width) {
        if (width)
            buffer[width - 1] = '-';
        if (width > 1)
            buffer[width - 2] = '-';
    } else
        memcpy(buffer + width - length, number, length);
    return buffer;
}

/* Recognize a numeric field in a display template. */
static bool number_placeholder(const char *text) {
    /* Each successful check also proves that byte is not the string terminator. */
    return text[0] == '$' && text[1] >= '0' && text[1] <= '9' && text[2] == '.' && text[3] >= '0' &&
           text[3] <= '9' && text[4] == 'f';
}

/* The caller provides DASHBOARD_MESSAGE_MAX_LENGTH + 1 output bytes. */

/* Build a readable screen from its labels, values and units. */
void dashboard_format_values(const char *template, const float *values, const uint8_t *paramId,
                             char *result) {
    size_t length = 0;
    unsigned element = 0;
    for (const char *text = template; *text && length < DASHBOARD_MESSAGE_MAX_LENGTH;) {
        bool numeric = number_placeholder(text);
        bool enumeration = !strncmp(text, "$enum", 5);
        if (element >= 4 || paramId[element] >= 100 || (!numeric && !enumeration)) {
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
            formatted = isfinite(values[element]) ? enum_text(id, values[element], symbol) : "--";
        }
        append_text(result, &length, formatted);
        ++element;
        text += 5;
        /* A run status is text, so it must not acquire the numeric seconds suffix. */
        if (formatted == statistics_labels[0] || formatted == statistics_labels[1]) {
            if (*text == ' ')
                ++text;
            if (*text == 's')
                ++text;
        }
    }
    result[length] = '\0';
}

#endif
