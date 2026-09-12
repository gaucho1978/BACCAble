#include "features/menu_input.h"
#include "test_report.h"
#include <assert.h>

static MenuInput press(uint8_t button, uint32_t now) {
    MenuInput input = {0};
    assert(menu_input_update(&input, 0x10, true, now) == MENU_NONE);
    assert(menu_input_update(&input, button, true, now) ==
           (button == 0x18 ? MENU_NEXT : MENU_PREVIOUS));
    return input;
}

static MenuEvent held(MenuInput *input, uint32_t now) {
    assert(menu_input_update(input, input->button, true, now) == MENU_NONE);
    return menu_input_repeat(input, true, now);
}

static void exact_boundaries_and_directions(void) {
    const uint8_t buttons[] = {0x18, 0x08};
    for (unsigned i = 0; i < sizeof(buttons); ++i) {
        MenuInput input = press(buttons[i], 100);
        MenuEvent expected = i == 0 ? MENU_NEXT : MENU_PREVIOUS;
        assert(held(&input, 350) == MENU_NONE);
        assert(held(&input, 599) == MENU_NONE);
        assert(held(&input, 600) == expected);
        assert(held(&input, 779) == MENU_NONE);
        assert(held(&input, 780) == expected);
        assert(held(&input, 1080) == expected);
        assert(held(&input, 1080) == MENU_NONE);
        assert(held(&input, 1259) == MENU_NONE);
        assert(held(&input, 1260) == expected);
    }
}

static void timer_wrap(void) {
    uint32_t start = UINT32_MAX - 300U;
    MenuInput input = press(0x18, start);
    assert(held(&input, start + 250U) == MENU_NONE);
    assert(held(&input, start + 499U) == MENU_NONE);
    assert(held(&input, start + 500U) == MENU_NEXT);
    assert(held(&input, start + 679U) == MENU_NONE);
    assert(held(&input, start + 680U) == MENU_NEXT);
}

static void release_and_direction_change_reset_delay(void) {
    MenuInput input = press(0x18, 100);
    assert(held(&input, 350) == MENU_NONE);
    assert(held(&input, 600) == MENU_NEXT);
    assert(menu_input_update(&input, 0x10, true, 700) == MENU_NONE);
    assert(menu_input_repeat(&input, true, 780) == MENU_NONE);
    assert(menu_input_update(&input, 0x18, true, 800) == MENU_NEXT);
    assert(held(&input, 1000) == MENU_NONE);
    assert(menu_input_update(&input, 0x08, true, 1100) == MENU_NONE);
    assert(held(&input, 1300) == MENU_NONE);
    assert(held(&input, 1599) == MENU_NONE);
    assert(held(&input, 1600) == MENU_PREVIOUS);
}

static void stale_stream_and_rearming(void) {
    MenuInput input = press(0x18, 100);
    assert(menu_input_update(&input, 0x18, true, 300) == MENU_NONE);
    assert(menu_input_repeat(&input, true, 600) == MENU_NEXT);
    input = press(0x18, 100);
    assert(menu_input_update(&input, 0x18, true, 300) == MENU_NONE);
    assert(menu_input_repeat(&input, true, 601) == MENU_NONE);
    assert(held(&input, 601) == MENU_NONE);
    assert(!input.armed);
    assert(held(&input, 801) == MENU_NONE);
    assert(menu_input_update(&input, 0x10, true, 900) == MENU_NONE);
    assert(menu_input_update(&input, 0x18, true, 1000) == MENU_NEXT);
    assert(held(&input, 1250) == MENU_NONE);
    assert(held(&input, 1500) == MENU_NEXT);
}

static void disabled_repeat_and_input(void) {
    MenuInput input = press(0x18, 100);
    assert(held(&input, 350) == MENU_NONE);
    assert(menu_input_update(&input, 0x18, true, 600) == MENU_NONE);
    assert(menu_input_repeat(&input, false, 600) == MENU_NONE);
    assert(menu_input_repeat(&input, true, 600) == MENU_NEXT);
    assert(menu_input_update(&input, 0x18, false, 780) == MENU_NONE);
    assert(menu_input_repeat(&input, true, 780) == MENU_NONE);
    assert(!input.armed);
}

static void other_buttons_never_repeat(void) {
    const uint8_t buttons[] = {0x10, 0x20, 0x00, 0x90, 0x50, 0x42};
    for (unsigned i = 0; i < sizeof(buttons); ++i) {
        MenuInput input = {0};
        assert(menu_input_update(&input, 0x10, true, 100) == MENU_NONE);
        menu_input_update(&input, buttons[i], true, 100);
        for (uint32_t now = 200; now <= 1600; now += 100) {
            menu_input_update(&input, buttons[i], true, now);
            assert(menu_input_repeat(&input, true, now) == MENU_NONE);
        }
    }
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(exact_boundaries_and_directions),
        HOST_TEST(timer_wrap),
        HOST_TEST(release_and_direction_change_reset_delay),
        HOST_TEST(stale_stream_and_rearming),
        HOST_TEST(disabled_repeat_and_input),
        HOST_TEST(other_buttons_never_repeat),
    };
    host_tests_run("input_repeat", tests, sizeof(tests) / sizeof(tests[0]));
    return 0;
}
