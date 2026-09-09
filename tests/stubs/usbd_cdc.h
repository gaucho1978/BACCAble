#ifndef TEST_USB_H
#define TEST_USB_H
#include "stm32f0xx_hal.h"
#define CDC_DATA_FS_MAX_PACKET_SIZE 64
#define USBD_OK 0
#define USBD_BUSY 1
#define USBD_FAIL 2
#define USBD_STATE_CONFIGURED 3
#define CDC_GET_LINE_CODING 0x21
typedef struct {
    volatile uint32_t TxState;
} USBD_CDC_HandleTypeDef;
typedef struct {
    void *pClassData;
    uint8_t dev_state;
} USBD_HandleTypeDef;
typedef struct {
    int8_t (*Init)(void);
    int8_t (*DeInit)(void);
    int8_t (*Control)(uint8_t, uint8_t *, uint16_t);
    int8_t (*Receive)(uint8_t *, uint32_t *);
} USBD_CDC_ItfTypeDef;
uint8_t USBD_CDC_SetTxBuffer(USBD_HandleTypeDef *, uint8_t *, uint16_t);
uint8_t USBD_CDC_SetRxBuffer(USBD_HandleTypeDef *, uint8_t *);
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *);
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *);
#endif
