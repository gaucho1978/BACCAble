#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
#include <stddef.h>
#define HAL_OK 0
#define HAL_ERROR 1
#define HAL_BUSY 2
#define ENABLE 1
#define DISABLE 0
#define CAN_ID_STD 0
#define CAN_ID_EXT 4
#define CAN_RTR_DATA 0
#define CAN_RTR_REMOTE 2
#define CAN_MODE_NORMAL 0
#define CAN_MODE_SILENT 1
#define CAN_RX_FIFO0 0
#define CAN_SJW_1TQ 0
#define CAN_BS1_4TQ 0
#define CAN_BS2_3TQ 0
#define CAN_FILTERMODE_IDMASK 0
#define CAN_FILTERSCALE_32BIT 0
#define CAN_MCR_RESET 1
#define GPIO_PIN_8 256
#define GPIO_PIN_9 512
#define GPIO_MODE_AF_PP 0
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_HIGH 0
#define GPIO_AF4_CAN 0
#define GPIOB NULL
#define CEC_CAN_IRQn 0
#define __HAL_RCC_CAN1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __DMB() ((void)0)
extern uint32_t fake_primask;
static inline uint32_t __get_PRIMASK(void) { return fake_primask; }
static inline void __disable_irq(void) { fake_primask = 1; }
static inline void __set_PRIMASK(uint32_t value) { fake_primask = value; }
typedef struct {
    uint32_t MCR;
} CAN_TypeDef;
extern CAN_TypeDef fake_can;
#define CAN (&fake_can)
typedef struct {
    uint32_t Pin, Mode, Pull, Speed, Alternate;
} GPIO_InitTypeDef;
typedef struct {
    uint32_t Prescaler, Mode, SyncJumpWidth, TimeSeg1, TimeSeg2, TimeTriggeredMode, AutoBusOff, AutoWakeUp,
        AutoRetransmission, ReceiveFifoLocked, TransmitFifoPriority;
} CAN_InitTypeDef;
typedef struct {
    CAN_TypeDef *Instance;
    CAN_InitTypeDef Init;
} CAN_HandleTypeDef;
typedef struct {
    uint32_t FilterIdHigh, FilterIdLow, FilterMaskIdHigh, FilterMaskIdLow, FilterFIFOAssignment, FilterBank,
        FilterMode, FilterScale, FilterActivation;
} CAN_FilterTypeDef;
typedef struct {
    uint32_t StdId, ExtId, IDE, RTR, DLC, TransmitGlobalTime;
} CAN_TxHeaderTypeDef;
typedef struct {
    uint32_t StdId, ExtId, IDE, RTR, DLC, Timestamp, FilterMatchIndex;
} CAN_RxHeaderTypeDef;
void HAL_GPIO_Init(void *, GPIO_InitTypeDef *);
void HAL_NVIC_SetPriority(int, int, int);
void HAL_NVIC_EnableIRQ(int);
uint32_t HAL_CAN_Init(CAN_HandleTypeDef *);
uint32_t HAL_CAN_ConfigFilter(CAN_HandleTypeDef *, CAN_FilterTypeDef *);
uint32_t HAL_CAN_Start(CAN_HandleTypeDef *);
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(CAN_HandleTypeDef *);
uint32_t HAL_CAN_AddTxMessage(CAN_HandleTypeDef *, CAN_TxHeaderTypeDef *, uint8_t *, uint32_t *);
uint32_t HAL_CAN_GetRxMessage(CAN_HandleTypeDef *, uint32_t, CAN_RxHeaderTypeDef *, uint8_t *);
uint32_t HAL_CAN_GetRxFifoFillLevel(CAN_HandleTypeDef *, uint32_t);
#endif
