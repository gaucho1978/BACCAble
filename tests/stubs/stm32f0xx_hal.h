#ifndef TEST_HAL_H
#define TEST_HAL_H
typedef struct {
    int unused;
} TIM_HandleTypeDef;
unsigned int HAL_GetTick(void);
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

typedef struct {
    uint32_t CR1;
} USART_TypeDef;
extern USART_TypeDef fake_usart1, fake_usart2;
#define USART1 (&fake_usart1)
#define USART2 (&fake_usart2)
typedef struct {
    uint32_t BaudRate, WordLength, StopBits, Parity, Mode, HwFlowCtl, OverSampling;
} UART_InitTypeDef;
typedef struct {
    USART_TypeDef *Instance;
    UART_InitTypeDef Init;
    struct {
        uint32_t AdvFeatureInit;
    } AdvancedInit;
    uint32_t TxXferCount, ErrorCode, State;
} UART_HandleTypeDef;
#define GPIOA NULL
#define GPIO_PIN_14 16384
#define GPIO_PIN_6 64
#define GPIO_MODE_AF_OD 0
#define GPIO_PULLUP 1
#define GPIO_SPEED_FREQ_MEDIUM 0
#define GPIO_AF1_USART2 0
#define GPIO_AF0_USART1 0
#define UART_WORDLENGTH_8B 0
#define UART_STOPBITS_1 0
#define UART_PARITY_NONE 0
#define UART_MODE_TX_RX 0
#define UART_HWCONTROL_NONE 0
#define UART_OVERSAMPLING_16 0
#define UART_ADVFEATURE_NO_INIT 0
#define USART2_IRQn 0
#define USART1_IRQn 0
#define UART_IT_TXE 1
#define UART_IT_TC 2
#define UART_IT_RXNE 4
#define UART_IT_ERR 8
#define UART_CLEAR_FEF 1
#define UART_CLEAR_NEF 2
#define UART_CLEAR_OREF 4
#define UART_CLEAR_PEF 8
#define HAL_UART_ERROR_NONE 0
#define HAL_UART_STATE_READY 0
#define USART_CR1_TE 1
#define USART_CR1_RE 2
#define CLEAR_BIT(reg, bit) ((reg) &= ~(bit))
#define SET_BIT(reg, bit) ((reg) |= (bit))
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART1_CLK_ENABLE() ((void)0)
#define __HAL_RCC_USART2_CLK_ENABLE() ((void)0)
#define __HAL_UART_DISABLE_IT(uart, flag) ((void)(uart), (void)(flag))
#define __HAL_UART_CLEAR_FLAG(uart, flag) ((void)(uart), (void)(flag))
#define __HAL_UART_FLUSH_DRREGISTER(uart) ((void)(uart))
uint32_t HAL_HalfDuplex_Init(UART_HandleTypeDef *);
uint32_t HAL_UART_Receive_IT(UART_HandleTypeDef *, uint8_t *, uint16_t);
uint32_t HAL_UART_Transmit_IT(UART_HandleTypeDef *, uint8_t *, uint16_t);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *);
void HAL_Delay(uint32_t);
#endif
