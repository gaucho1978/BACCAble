//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "transport/can_bus.h"
#include <string.h>

// Private variables
static CAN_HandleTypeDef can_handle;
static uint32_t prescaler;
static can_bus_state_t bus_state = OFF_BUS;
static uint8_t can_autoretransmit = ENABLE;
static uint32_t can_mode = CAN_MODE_NORMAL;
static can_txbuf_t txqueue = {0};

// Initialize CAN peripheral settings, but don't actually start the peripheral
void can_init(void) {
    // Initialize GPIO for CAN transceiver
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // PB8     ------> CAN_RX
    // PB9     ------> CAN_TX
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_CAN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // default to 500 kbit/s
    prescaler = 12;
    can_handle.Instance = CAN;
    bus_state = OFF_BUS;

    HAL_NVIC_SetPriority(CEC_CAN_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CEC_CAN_IRQn);
}

// Start the CAN peripheral
void can_enable(void) {
    if (bus_state == OFF_BUS) {
        can_handle.Init.Prescaler = prescaler;
        can_handle.Init.Mode = can_mode;

        can_handle.Init.SyncJumpWidth = CAN_SJW_1TQ;
        can_handle.Init.TimeSeg1 = CAN_BS1_4TQ;
        can_handle.Init.TimeSeg2 = CAN_BS2_3TQ;
        can_handle.Init.TimeTriggeredMode = DISABLE;
        can_handle.Init.AutoBusOff = ENABLE;
        can_handle.Init.AutoWakeUp = DISABLE;
        can_handle.Init.AutoRetransmission = can_autoretransmit;
        can_handle.Init.ReceiveFifoLocked = DISABLE;
        can_handle.Init.TransmitFifoPriority = ENABLE;
        if (HAL_CAN_Init(&can_handle) != HAL_OK)
            return;

        CAN_FilterTypeDef filter = {0};
        filter.FilterIdHigh = 0;
        filter.FilterIdLow = 0;
        filter.FilterMaskIdHigh = 0;
        filter.FilterMaskIdLow = 0;
        filter.FilterFIFOAssignment = CAN_RX_FIFO0;
        filter.FilterBank = 0;
        filter.FilterMode = CAN_FILTERMODE_IDMASK;
        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterActivation = ENABLE;
        if (HAL_CAN_ConfigFilter(&can_handle, &filter) != HAL_OK)
            return;

        if (HAL_CAN_Start(&can_handle) != HAL_OK)
            return;
        bus_state = ON_BUS;

        status_led_activity();
    }
}

// Disable the CAN peripheral and go off-bus
void can_disable(void) {
    if (bus_state == ON_BUS) {
        // Do a bxCAN reset (set RESET bit to 1)
        can_handle.Instance->MCR |= CAN_MCR_RESET;
        bus_state = OFF_BUS;
        memset(&txqueue, 0, sizeof(txqueue));

        status_led_error();
    }
}

// Set the bitrate of the CAN peripheral
void can_set_bitrate(enum can_bitrate bitrate) {
    if (bus_state == ON_BUS) {
        // cannot set bitrate while on bus
        return;
    }

    switch (bitrate) {
    case CAN_BITRATE_10K:
        prescaler = 600;
        break;
    case CAN_BITRATE_20K:
        prescaler = 300;
        break;
    case CAN_BITRATE_50K:
        prescaler = 120;
        break;
    case CAN_BITRATE_100K:
        prescaler = 60;
        break;
    case CAN_BITRATE_125K:
        prescaler = 48;
        break;
    case CAN_BITRATE_250K:
        prescaler = 24;
        break;
    case CAN_BITRATE_500K:
        prescaler = 12;
        break;
    case CAN_BITRATE_750K:
        prescaler = 8;
        break;
    case CAN_BITRATE_1000K:
        prescaler = 6;
        break;
    case CAN_BITRATE_INVALID:
    default:
        return;
    }

    status_led_activity();
}

// Set CAN peripheral to silent mode
void can_set_silent(uint8_t silent) {
    if (bus_state == ON_BUS) {
        // cannot set silent mode while on bus
        return;
    }
    if (silent) {
        can_mode = CAN_MODE_SILENT;
    } else {
        can_mode = CAN_MODE_NORMAL;
    }

    status_led_error();
}

// Enable/disable auto-retransmission
void can_set_autoretransmit(uint8_t autoretransmit) {
    if (bus_state == ON_BUS) {
        // Cannot set autoretransmission while on bus
        return;
    }
    if (autoretransmit) {
        can_autoretransmit = ENABLE;
    } else {
        can_autoretransmit = DISABLE;
    }

    status_led_error();
}

// Send a message on the CAN bus
uint32_t can_tx(CAN_TxHeaderTypeDef *tx_msg_header, uint8_t *tx_msg_data) {
    if (bus_state != ON_BUS || !tx_msg_header || !tx_msg_data || tx_msg_header->DLC > 8 ||
        (tx_msg_header->IDE != CAN_ID_STD && tx_msg_header->IDE != CAN_ID_EXT) ||
        (tx_msg_header->RTR != CAN_RTR_DATA && tx_msg_header->RTR != CAN_RTR_REMOTE) ||
        (tx_msg_header->IDE == CAN_ID_STD && tx_msg_header->StdId > 0x7ff) ||
        (tx_msg_header->IDE == CAN_ID_EXT && tx_msg_header->ExtId > 0x1fffffff))
        return HAL_ERROR;
    // One slot stays empty to distinguish a full queue from an empty one.
    if (((txqueue.head + 1) % TXQUEUE_LEN) == txqueue.tail) {
        error_assert(ERR_FULLBUF_CANTX);
        return HAL_ERROR;
    }

    // Copy header struct into array
    txqueue.header[txqueue.head] = *tx_msg_header;

    // Copy data into array
    for (uint8_t i = 0; i < tx_msg_header->DLC; i++) {
        txqueue.data[txqueue.head][i] = tx_msg_data[i];
    }

    // Increment the head pointer
    txqueue.head = (txqueue.head + 1) % TXQUEUE_LEN;

    return HAL_OK;
}

// Process messages in the TX output queue
void can_process(void) {
    if (bus_state != ON_BUS)
        return;
    while ((txqueue.tail != txqueue.head) && (HAL_CAN_GetTxMailboxesFreeLevel(&can_handle) > 0)) {
        // Transmit can frame
        uint32_t mailbox_txed = 0;
        uint32_t status = HAL_CAN_AddTxMessage(&can_handle, &txqueue.header[txqueue.tail],
                                               txqueue.data[txqueue.tail], &mailbox_txed);

        // status_led_error();

        // Retain the queued frame until the peripheral accepts it.
        if (status != HAL_OK) {
            error_assert(ERR_CAN_TXFAIL);
            return;
        }
        txqueue.tail = (txqueue.tail + 1) % TXQUEUE_LEN;
    }
}

// Receive message from the CAN bus RXFIFO
uint32_t can_rx(CAN_RxHeaderTypeDef *rx_msg_header, uint8_t *rx_msg_data) {
    if (bus_state != ON_BUS || !rx_msg_header || !rx_msg_data)
        return HAL_ERROR;
    memset(rx_msg_data, 0, 8);
    uint32_t status = HAL_CAN_GetRxMessage(&can_handle, CAN_RX_FIFO0, rx_msg_header, rx_msg_data);
    // status_led_activity(); disabled to avoid too much lights, but you uncomment it for debug
    return status == HAL_OK && rx_msg_header->DLC <= 8 ? HAL_OK : HAL_ERROR;
}

// Check if a CAN message has been received and is waiting in the FIFO
uint8_t is_can_msg_pending(uint8_t fifo) {
    if (bus_state == OFF_BUS) {
        return 0;
    }
    return (HAL_CAN_GetRxFifoFillLevel(&can_handle, fifo) > 0);
}

uint32_t can_forward(const CAN_RxHeaderTypeDef *received, uint8_t *data) {
    if (!received)
        return HAL_ERROR;
    CAN_TxHeaderTypeDef header = {.StdId = received->StdId,
                                  .ExtId = received->ExtId,
                                  .IDE = received->IDE,
                                  .RTR = received->RTR,
                                  .DLC = received->DLC,
                                  .TransmitGlobalTime = DISABLE};
    return can_tx(&header, data);
}

// Return reference to CAN handle
CAN_HandleTypeDef *can_gethandle(void) { return &can_handle; }

// Callback for FIFO0 full
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
    (void)hcan;
    error_assert(ERR_CANRXFIFO_OVERFLOW);
}
