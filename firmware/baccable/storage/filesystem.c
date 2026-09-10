#include "platform/system.h"
#ifdef ENABLE_USB_MASS_STORAGE
static FATFS fs;
#endif
/* Write the legacy demonstration file; this is not a vehicle-traffic recorder. */
void filesystem_save_log(void) {
#ifdef ENABLE_USB_MASS_STORAGE

    FIL fil;
    UINT bw;
    FRESULT res;

    res = f_mount(&fs, "", 1);
    if (res == FR_OK) {
        res = f_open(&fil, "hello.txt", FA_WRITE | FA_OPEN_ALWAYS);
        if (res == FR_OK) {
            f_write(&fil, "Hello, World!\r\n", 15, &bw);
            f_sync(&fil);
            f_close(&fil);
            status_led_activity();
        }
    }
    f_unmount("");

#endif
}

/* Prepare an unformatted device disk and identify the board's firmware. */
void filesystem_init(void) {

#ifdef ENABLE_USB_MASS_STORAGE
    // FATFS fs;
    FIL fil;
    UINT bw;
    FRESULT res;
    BYTE work[FF_MIN_SS];

    res = f_mount(&fs, "", 1);
    if (res == FR_NO_FILESYSTEM) {

        MKFS_PARM opt = {.fmt = FM_FAT | FM_SFD, .n_fat = 1, .align = 0, .n_root = 32, .au_size = FF_MIN_SS};
        res = f_mkfs("", &opt, work, FF_MIN_SS);
        if (res == FR_OK) {
            res = f_setlabel("BACCABLE "
    #ifdef ACT_AS_CANABLE
                             "Sniffer"
    #elif defined(BACCABLE_C1)
                             "C1"
    #elif defined(BACCABLE_C2)
                             "C2"
    #elif defined(BACCABLE_BH)
                             "BH"
    #endif
            );
            res = f_open(&fil, "Version.txt", FA_CREATE_ALWAYS | FA_WRITE);

            if (res == FR_OK) {
                status_led_activity();

                f_write(&fil, _FW_VERSION, strlen(_FW_VERSION), &bw);
                f_close(&fil);
            } else {
                status_led_error();
            }
        }
    }

    res = f_unmount("");

#endif
}
