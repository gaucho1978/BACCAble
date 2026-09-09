#include "features/value_format.h"
uint8_t visible_page_find(const uint8_t *visible, uint8_t count, uint8_t start, int direction) {
    if (!visible || !count)
        return 0;
    unsigned page = start < count ? start : (direction < 0 ? count - 1 : 0);
    for (unsigned visited = 0; visited < count; ++visited) {
        if (visible[page])
            return (uint8_t)page;
        page = direction < 0 ? (page + count - 1) % count : (page + 1) % count;
    }
    return 0;
}
