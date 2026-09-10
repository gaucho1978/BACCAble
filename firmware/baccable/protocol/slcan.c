#include "protocol/slcan.h"
#include "protocol/slcan_codec.h"
#include "usbd_cdc_if.h"

extern const char *FW_VERSION;

/* Prepare a received CAN frame for a connected host application. */
int8_t slcan_parse_frame(uint8_t *text, CAN_RxHeaderTypeDef *header, uint8_t *data) {
    if (header->DLC > 8)
        return 0;
    CanFrame frame = {.id = header->IDE == CAN_ID_EXT ? header->ExtId : header->StdId,
                      .extended = header->IDE == CAN_ID_EXT,
                      .remote = header->RTR == CAN_RTR_REMOTE,
                      .length = header->DLC};
    memcpy(frame.data, data, frame.length);
    return (int8_t)slcan_encode(&frame, text, SLCAN_MTU);
}

/* Apply a supported host command to the CAN adapter. */
int8_t slcan_parse_str(uint8_t *text, uint8_t length) {
    if (!text || !length)
        return -1;
    if (length == 1) {
        switch (text[0]) {
        case 'O':
            can_enable();
            return 0;
        case 'C':
            can_disable();
            return 0;
        case 'V':
            return CDC_Transmit_FS((uint8_t *)FW_VERSION, strlen(FW_VERSION)) == USBD_OK ? 0 : -1;
        case 'E': {
            char status[64];
            int size = snprintf_(status, sizeof(status), "CANable Error Register: %X", (unsigned)error_reg());
            return CDC_Transmit_FS((uint8_t *)status, size) == USBD_OK ? 0 : -1;
        }
        default:
            return -1;
        }
    }
    if (length == 2) {
        if (text[0] == 'S' && text[1] >= '0' && text[1] <= '8') {
            can_set_bitrate((enum can_bitrate)(text[1] - '0'));
            return 0;
        }
        if (text[1] != '0' && text[1] != '1')
            return -1;
        switch (text[0]) {
        case 'm':
        case 'M':
            can_set_silent(text[1] == '1');
            return 0;
        case 'a':
        case 'A':
            can_set_autoretransmit(text[1] == '1');
            return 0;
        default:
            return -1;
        }
    }
    CanFrame frame;
    if (!slcan_decode(text, length, &frame))
        return -1;
    CAN_TxHeaderTypeDef header = {.IDE = frame.extended ? CAN_ID_EXT : CAN_ID_STD,
                                  .RTR = frame.remote ? CAN_RTR_REMOTE : CAN_RTR_DATA,
                                  .StdId = frame.extended ? 0 : frame.id,
                                  .ExtId = frame.extended ? frame.id : 0,
                                  .DLC = frame.length};
    return can_tx(&header, frame.data) == HAL_OK ? 0 : -1;
}
