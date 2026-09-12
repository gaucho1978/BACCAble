#include "test_report.h"
#include "protocol/slcan_codec.h"
#include "diagnostics/uds_decode.h"
#include "storage/record_store.h"
#include "storage/flash_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_slcan(void) {
    CanFrame frame;
    assert(slcan_decode((const uint8_t *)"t1232AAbb", 9, &frame));
    assert(frame.id == 0x123 && frame.length == 2 && frame.data[1] == 0xbb);
    const char *invalid[] = {"",        "t",     "t1",         "t123",    "t1239",     "t1232AA",
                             "t1231GG", "t8000", "T200000000", "r1231AA", "t1230extra"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(!slcan_decode((const uint8_t *)invalid[i], strlen(invalid[i]), &frame));
    uint8_t text[SLCAN_TEXT_CAPACITY];
    for (unsigned extended = 0; extended < 2; ++extended) {
        for (unsigned remote = 0; remote < 2; ++remote) {
            for (unsigned dlc = 0; dlc <= 8; ++dlc) {
                CanFrame sent = {.id = extended ? 0x1fffffff : 0x7ff,
                                 .length = dlc,
                                 .extended = extended,
                                 .remote = remote,
                                 .data = {0, 1, 0x7f, 0x80, 0xaa, 0xbb, 0xfe, 0xff}};
                size_t length = slcan_encode(&sent, text, sizeof(text));
                assert(length && text[length - 1] == '\r');
                assert(!slcan_encode(&sent, text, length - 1));
                assert(slcan_decode(text, length - 1, &frame));
                assert(frame.id == sent.id && frame.length == dlc && frame.remote == remote &&
                       frame.extended == extended);
                if (!remote)
                    assert(!memcmp(frame.data, sent.data, dlc));
            }
        }
    }
    /* Every truncation of a maximum-length frame must fail without reading past input. */
    const char *full = "T18DAF11080011223344556677";
    for (size_t length = 0; length < strlen(full); ++length) {
        uint8_t input[27];
        memcpy(input, full, length);
        assert(!slcan_decode(input, length, &frame));
    }
}

static void test_uds(void) {
    uint8_t reply[8] = {5, 0x62, 0x19, 0x59, 0x7f, 0xff, 0, 0};
    float value = 123;
    assert(uds_decode_value(reply, 8, 0x1959, 0, 2, -32768, 0.001f, -1, &value));
    assert(fabsf(value + 1.001f) < 0.00001f);
    assert(!uds_decode_value(reply, 5, 0x1959, 0, 2, 0, 1, 0, &value));
    assert(!uds_decode_value(reply, 8, 0x1958, 0, 2, 0, 1, 0, &value));
    assert(!uds_decode_value(reply, 8, 0x1959, 255, 4, 0, 1, 0, &value));
    reply[1] = 0x7f;
    assert(!uds_decode_value(reply, 8, 0x1959, 0, 2, 0, 1, 0, &value));
    reply[1] = 0x62;
    reply[0] = 0x10;
    assert(!uds_decode_value(reply, 8, 0x1959, 0, 2, 0, 1, 0, &value));
}

typedef struct {
    uint8_t pages[2][256];
    int remaining;
    unsigned erases;
} FakeFlash;
static bool erase(void *context, unsigned page) {
    FakeFlash *flash = context;
    if (flash->remaining-- == 0)
        return false;
    memset(flash->pages[page], 0xff, sizeof(flash->pages[page]));
    ++flash->erases;
    return true;
}
static bool program(void *context, unsigned page, size_t offset, uint16_t value) {
    FakeFlash *flash = context;
    assert(offset + 1 < sizeof(flash->pages[page]));
    if (flash->remaining-- == 0)
        return false;
    flash->pages[page][offset] &= value;
    flash->pages[page][offset + 1] &= value >> 8;
    return true;
}
static void test_records(void) {
    FakeFlash flash;
    memset(&flash, 0xff, sizeof(flash));
    flash.erases = 0;
    RecordStorage storage = {
        .pages = {flash.pages[0], flash.pages[1]}, .erase = erase, .program = program, .context = &flash};
    uint8_t before[] = {1, 2, 3, 4, 5};
    uint8_t after[] = {9, 8, 7, 6, 5};
    uint8_t loaded[5];
    assert(!record_load(&storage, 1, loaded, sizeof(loaded)));
    assert(record_save(&storage, 1, before, sizeof(before)));
    FakeFlash baseline = flash;
    for (int failure = 0; failure < 15; ++failure) {
        flash = baseline;
        flash.remaining = failure;
        bool saved = record_save(&storage, 1, after, sizeof(after));
        assert(record_load(&storage, 1, loaded, sizeof(loaded)));
        assert(!memcmp(loaded, saved ? after : before, sizeof(loaded)));
        /* Saving the recoverable value again must not touch Flash after an interrupted save. */
        flash.remaining = 0;
        unsigned erases = flash.erases;
        assert(record_save(&storage, 1, loaded, sizeof(loaded)));
        assert(flash.remaining == 0 && flash.erases == erases);
    }
    flash = baseline;
    flash.remaining = -1;
    assert(record_save(&storage, 1, after, sizeof(after)));
    unsigned erases = flash.erases;
    flash.remaining = 0; /* Any erase or program attempt would fail and consume this counter. */
    assert(record_save(&storage, 1, after, sizeof(after)) && flash.erases == erases);
    assert(flash.remaining == 0);
    flash.pages[1][RECORD_HEADER_SIZE] ^= 1;
    assert(record_load(&storage, 1, loaded, sizeof(loaded)));
    assert(!memcmp(loaded, before, sizeof(loaded)));
    assert(!record_load(&storage, 2, loaded, sizeof(loaded)));
    assert(!record_load(&storage, 1, loaded, sizeof(loaded) - 1));
    assert(!storage_sector_range(UINT32_MAX, 1));
    assert(!storage_sector_range(0, UINT32_MAX));
    assert(!storage_sector_range(STORAGE_SECTOR_COUNT, 1));
    assert(storage_sector_range(STORAGE_SECTOR_COUNT - 1, 1));
    assert(USB_FLASH_START_ADDRESS + TOTAL_USB_DEVICE_SIZE == STORAGE_RECORDS_START);
    assert(STORAGE_RECORDS_START + 6 * STORAGE_PAGE_SIZE == STORAGE_FLASH_END);
}

int main(void) {
    const HostTest tests[] = {
        HOST_TEST(test_slcan),
        HOST_TEST(test_uds),
        HOST_TEST(test_records)
    };
    host_tests_run("core", tests, sizeof(tests) / sizeof(tests[0]));
    puts("PASS: SLCAN, UDS, interrupted Flash writes, storage bounds");
    return 0;
}
