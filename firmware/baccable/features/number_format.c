#include "features/value_format.h"
#include "third_party/printf/printf.h"
#include <math.h>
#include <string.h>

void format_number(char *text, float value, uint8_t precision, uint8_t capacity) {
    if (!text || capacity == 0)
        return;
    memset(text, ' ', capacity - 1);
    text[capacity - 1] = 0;
    if (isnan(value))
        return;
    if (precision > 6)
        precision = 6;
    char number[48];
    int length = snprintf_(number, sizeof(number), "%.*f", precision, (double)value);
    if (length < 0 || length >= (int)sizeof(number))
        return;
    if (precision && isfinite(value)) {
        while (length && number[length - 1] == '0')
            number[--length] = 0;
        if (length && number[length - 1] == '.')
            number[--length] = 0;
    }
    size_t available = capacity - 1;
    memcpy(text, number, (size_t)length < available ? (size_t)length : available);
}
