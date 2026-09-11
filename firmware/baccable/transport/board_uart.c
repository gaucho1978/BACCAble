#include "transport/board_uart.h"
#include "stm32f0xx_hal.h"
#include "app/main.h"
#include "diagnostics/parameter_cache.h"

// extern void Error_Handler(void);

extern void board_commands_dispatch(const uint8_t *message);
#define UART_RX_SLOTS 5
static uint8_t rx_frames[UART_RX_SLOTS][UART_BUFFER_SIZE];
static volatile uint8_t rx_head, rx_tail;
#if !defined(ACT_AS_CANABLE)
static uint8_t active_tx[UART_BUFFER_SIZE];
#endif
#if defined(BACCABLE_C1)
static uint8_t active_pedal_tx[UART1_BUFFER_SIZE];
static uint8_t pending_screen[UART_BUFFER_SIZE];
static uint8_t screen_pending;
static uint8_t screen_overtook_poll;
#endif
static volatile uint8_t board_tx_active, pedal_tx_active;
static volatile uint8_t pedal_response, pedal_response_pending;
#define QUEUE_SIZE 10 // max queue size

typedef struct {
    uint8_t tx_buffer[QUEUE_SIZE][UART_BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} SendQueue;

static SendQueue queue_instance = {0};
static SendQueue *tx_queue = &queue_instance;

static uint32_t last_sent_serial_msg_time = 0;

#if defined(BACCABLE_C1)
static uint32_t last_c2_poll_time = 0;
static uint32_t last_bh_poll_time = 0;
#endif

static uint8_t board_rx_buffer[UART_BUFFER_SIZE]; // buffer to receive the message from uart

#if defined(BACCABLE_C1)
uint32_t last_pedal_tx_time = 0;
uint8_t pedal_rx_buffer[UART1_BUFFER_SIZE]; // buffer to receive the message from uart (schizzaForte)
SendQueue queue_instance_uart1 = {0};
SendQueue *tx_queue_uart1 = &queue_instance_uart1;

#endif

static uint8_t frame_synchronized = 0;
static uint8_t diagnostic_mode;

/* Give diagnostic traffic exclusive ownership of the shared board link. */
void board_uart_set_diagnostic(uint8_t enabled) {
    diagnostic_mode = !!enabled;
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    queue_instance.head = queue_instance.tail = queue_instance.count = 0;
#if defined(BACCABLE_C1)
    screen_pending = 0;
#endif
    __set_PRIMASK(irq);
}

/* Send one diagnostic frame and listen immediately for its reply. */
uint8_t board_uart_diagnostic_send(const uint8_t *data, size_t length) {
#if !defined(ACT_AS_CANABLE)
    if (!data || length != UART_BUFFER_SIZE)
        return 0;
    uint32_t started = currentTime;
    while (board_tx_active && currentTime - started < 30) {
    }
    if (board_tx_active)
        return 0;
    uart_pause(&huart2);
    CLEAR_BIT(huart2.Instance->CR1, USART_CR1_RE);
    SET_BIT(huart2.Instance->CR1, USART_CR1_TE);
    huart2.State = HAL_UART_STATE_READY;
    HAL_StatusTypeDef result = HAL_UART_Transmit(&huart2, (uint8_t *)data, length, 30);
    SET_BIT(huart2.Instance->CR1, USART_CR1_RE);
    uart_resume(&huart2);
    return result == HAL_OK;
#else
    (void)data;
    (void)length;
    return 0;
#endif
}

/* Prepare communication with auxiliary boards and the pedal controller. */
void uart_init() {

    __HAL_RCC_GPIOA_CLK_ENABLE(); // enable clock for Uart2

    // Configure PA14 as Half-Duplex TX/RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Open-Drain to support both TX and RX on the same pin
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // Necessary to avoid floating states
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#if defined(BACCABLE_C1)
    __HAL_RCC_GPIOB_CLK_ENABLE(); // enable clock for Uart1 (to schizzaforte)

    // Configure PB6 as Half-Duplex TX/RX (to schizzaforte)
    GPIO_InitStruct.Pin = GPIO_PIN_6;

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#endif

    HAL_Delay(4); // let's wait that electric signal on serial line get stabilized.

    __HAL_RCC_USART2_CLK_ENABLE(); // enable clock for usart2

    // Configure USART2 in Half-Duplex mode
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 38400; // 38400;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX; // Enable both TX and RX
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_HalfDuplex_Init(&huart2) != HAL_OK)
        Error_Handler(500); // error

    // **Enable the interrupt in the NVIC (if not already set in CubeMX)**
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    // **Enable reception in interrupt mode**
    HAL_UART_Receive_IT(&huart2, &board_rx_buffer[0], 1); // Start receiving one byte

#if defined(BACCABLE_C1)
    __HAL_RCC_USART1_CLK_ENABLE(); // enable clock for usart1 (to schizzaforte)

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX; // richiesto anche in half-duplex
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    //(void)tmp;

    if (HAL_HalfDuplex_Init(&huart1) != HAL_OK)
        Error_Handler(750);

    // **Enable the interrupt in the NVIC (if not already set in CubeMX)**
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    // **Enable reception in interrupt mode**
    HAL_UART_Receive_IT(&huart1, &pedal_rx_buffer[0], 1); // Start receiving one byte

#endif
}

/* Collect received board messages and pedal-controller responses for normal processing. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
#if defined(BACCABLE_C1)
    if (huart->Instance == USART1) { // message from schizzaForte
        // record current mode
        pedal_response = pedal_rx_buffer[0];
        pedal_response_pending = 1;
        // pedal_rx_buffer[0]
        status_led_activity();
        HAL_UART_Receive_IT(&huart1, &pedal_rx_buffer[0], 1); // receive one char
    }

#endif

    if (huart->Instance == USART2) { // message from other baccable chips
        // evaluate received message
        if ((board_rx_buffer[0] >= C1BusID) &&
            (board_rx_buffer[0] <= 0x10)) { // if the received char indicates the beginning of a message
            if (frame_synchronized) { // if we were sync, we can process the message, since the first char is
                                      // correct and the sync indicates that te remaining part too is complete
#if defined(ACT_AS_CANABLE)
                status_led_activity();
#endif

                if (board_rx_buffer[0] >= 0x0e) {
                    uint8_t checksum = 0;
                    for (unsigned i = 0; i < 17; ++i)
                        checksum ^= board_rx_buffer[i];
                    if (checksum != board_rx_buffer[17] || board_rx_buffer[7] > 8) {
                        frame_synchronized = 0;
                        HAL_UART_Receive_IT(&huart2, board_rx_buffer, 1);
                        return;
                    }
                }
                uint8_t next = (rx_head + 1) % UART_RX_SLOTS;
                if (next != rx_tail) {
                    memcpy(rx_frames[rx_head], board_rx_buffer, UART_BUFFER_SIZE);
                    __DMB();
                    rx_head = next;
                } else {
                    status_led_error();
                }

                HAL_UART_Receive_IT(&huart2, &board_rx_buffer[0],
                                    UART_BUFFER_SIZE); // receive next frame
            } else { // otherwise we were not sync, therefore we need to receive the remaining part of the
                     // message
                frame_synchronized = 1;
                HAL_UART_Receive_IT(&huart2, &board_rx_buffer[1],
                                    UART_BUFFER_SIZE - 1); // receive remaining part of the frame
            }
        } else {                    // we did not receive the begin of the message. discard it
            frame_synchronized = 0; // we lost sync

            HAL_UART_Receive_IT(&huart2, &board_rx_buffer[0], 1); // receive one char
        }
    }
}

/* Release a completed transmission and resume listening for replies. */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

    if (huart->Instance == USART2) {
        board_tx_active = 0;
        // message sent. what do we do?
        status_led_activity(); // successfully sent
    }

#if defined(BACCABLE_C1)
    if (huart->Instance == USART1) {
        pedal_tx_active = 0;

        CLEAR_BIT(huart1.Instance->CR1, USART_CR1_TE);        // disable TX
        SET_BIT(huart1.Instance->CR1, USART_CR1_RE);          // enable RX
        HAL_UART_Receive_IT(&huart1, &pedal_rx_buffer[0], 1); // restart RX
    }
#endif
}

/* Suspend communication while the board cannot safely receive or transmit. */
void uart_pause(UART_HandleTypeDef *huart) {
    // Abort interrupt-driven TX before releasing its buffer or resetting HAL state.
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
    __HAL_UART_DISABLE_IT(huart, UART_IT_TC);
    huart->TxXferCount = 0;
    if (huart->Instance == USART2)
        board_tx_active = 0;
    if (huart->Instance == USART1)
        pedal_tx_active = 0;
    __HAL_UART_DISABLE_IT(huart, UART_IT_RXNE); // disable RX interrupt (RXNE)
    __HAL_UART_DISABLE_IT(huart, UART_IT_ERR); // disable error interrupts (ORE overrun, FE framing, NE noise)

    // deletes all UARTS errors
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_PEF);

    __HAL_UART_FLUSH_DRREGISTER(huart); // Clear RXNE by flushing the data register
    __set_PRIMASK(irq);
}

/* Resume communication and wait for a fresh message boundary. */
void uart_resume(UART_HandleTypeDef *huart) {
    frame_synchronized = 0; // sync lost
    // Clear RXNE by flushing the data register
    __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_PEF);
    __HAL_UART_FLUSH_DRREGISTER(huart);

    // Reset HAL internal state
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->State = HAL_UART_STATE_READY;

#if defined(BACCABLE_C1)
    if (huart->Instance == USART1) {

        HAL_UART_Receive_IT(huart, &pedal_rx_buffer[0], 1); // restart receiving one byte
        last_pedal_tx_time =
            currentTime; // avoid to send message in the same moment when interrupt was restarted
    }
#endif

    if (huart->Instance == USART2) {
        HAL_UART_Receive_IT(huart, &board_rx_buffer[0], 1); // restart receiving one byte
        last_sent_serial_msg_time =
            currentTime; // avoid to send message in the same moment when interrupt was restarted
    }
}

/* Recover communication after an error when the device is allowed to listen. */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {

    if ((currentTime - runtime_state.last_uart_error_callback) > 1000)
        status_led_error();
    runtime_state.last_uart_error_callback = currentTime;

    uart_pause(huart);
    if ((runtime_state.low_consume_is_active == 0) || (huart->Instance == USART1))
        uart_resume(huart);
}

/* Keep a complete outgoing command until it can be transmitted. */
static uint8_t queue_send(SendQueue *queue, const uint8_t *data, size_t length, size_t frame_length) {
    if (!data || !length || length > frame_length) {
        status_led_error();
        return 0;
    }
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    uint8_t accepted = queue->count < QUEUE_SIZE;
    if (accepted) {
        memset(queue->tx_buffer[queue->tail], ' ', frame_length);
        memcpy(queue->tx_buffer[queue->tail], data, length);
        queue->tail = (queue->tail + 1) % QUEUE_SIZE;
        ++queue->count;
    } else {
        status_led_error();
    }
    __set_PRIMASK(irq);
    return accepted;
}

#if !defined(ACT_AS_CANABLE)
/* Send the next eligible command or latest screen without overwriting an active transmission. */
static uint8_t queue_start(SendQueue *queue, UART_HandleTypeDef *uart, uint8_t *active_buffer, size_t length,
                           volatile uint8_t *active) {
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    uint8_t started = 0;
    uint8_t use_screen = 0;
    #if defined(BACCABLE_C1)
    if (queue == tx_queue && screen_pending) {
        /* Display replaces old display only; vehicle commands retain FIFO order. */
        use_screen = !queue->count ||
                     (!screen_overtook_poll && (queue->tx_buffer[queue->head][0] == BhBusIDgetStatus ||
                                                (queue->tx_buffer[queue->head][0] == C2BusID &&
                                                 queue->tx_buffer[queue->head][1] == C2cmdGetStatus)));
    }
    #endif
    if (!*active && (queue->count || use_screen)) {
    #if defined(BACCABLE_C1)
        if (use_screen)
            memcpy(active_buffer, pending_screen, length);
        else
    #endif
            memcpy(active_buffer, queue->tx_buffer[queue->head], length);
        if (HAL_UART_Transmit_IT(uart, active_buffer, length) == HAL_OK) {
            *active = 1;
    #if defined(BACCABLE_C1)
            /* At most one display may overtake a queued poll, even during continuous scrolling. */
            if (queue == tx_queue)
                screen_overtook_poll = use_screen && queue->count;
            if (use_screen)
                screen_pending = 0;
            else
    #endif
            {
                queue->head = (queue->head + 1) % QUEUE_SIZE;
                --queue->count;
            }
            started = 1;
        }
    }
    __set_PRIMASK(irq);
    return started;
}

#endif

/* Queue a board command or replace an older waiting screen update. */
uint8_t board_uart_send(const uint8_t *data, size_t length) {
#if defined(BACCABLE_C1)
    if (data && length && length <= UART_BUFFER_SIZE && data[0] == BhBusIDparamString) {
        uint32_t irq = __get_PRIMASK();
        __disable_irq();
        memset(pending_screen, ' ', sizeof(pending_screen));
        memcpy(pending_screen, data, length);
        screen_pending = 1;
        __set_PRIMASK(irq);
        return 1;
    }
#endif
    return queue_send(tx_queue, data, length, UART_BUFFER_SIZE);
}

#if defined(BACCABLE_C1)
/* Queue a request for the pedal controller. */
void pedal_uart_send(const uint8_t *data, size_t length) {
    queue_send(tx_queue_uart1, data, length, UART1_BUFFER_SIZE);
}
/* Apply received pedal status and send the next request when a reply can be expected. */
void pedal_uart_process(void) {
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    uint8_t pending = pedal_response_pending;
    uint8_t response = pedal_response;
    pedal_response_pending = 0;
    __set_PRIMASK(irq);
    if (pending) {
        switch (response) { // 0x10=Bypass, 0x59=All weather, 0xa2=Natural, 0xeb=Dynamic, 0x34=Race
        case 0x10:
            pedal_state.current_schizzaforte_map = 'B';
            break;
        case 0x59:
            pedal_state.current_schizzaforte_map = 'A';
            break;
        case 0xa2:
            pedal_state.current_schizzaforte_map = 'N';
            break;
        case 0xeb:
            pedal_state.current_schizzaforte_map = 'D';
            break;
        case 0x34:
            pedal_state.current_schizzaforte_map = 'R';
            break;
        default:
            pedal_state.current_schizzaforte_map = '?';
            break;
        }
    }
    if (pending && pedal_state.current_schizzaforte_map != '?')
        parameter_cache_put(17, native_parameter_read(17), currentTime);
    if (currentTime < TIMING__ALL___SERIAL_IGNORE_WINDOW_MS || pedal_tx_active || !tx_queue_uart1->count)
        return;
    if (currentTime - last_pedal_tx_time <= TIMING__C1____SCHIZZAFORTE_SERIAL_TIMEOUT_REPLY_MS)
        return;
    CLEAR_BIT(huart1.Instance->CR1, USART_CR1_RE);
    SET_BIT(huart1.Instance->CR1, USART_CR1_TE);
    if (queue_start(tx_queue_uart1, &huart1, active_pedal_tx, UART1_BUFFER_SIZE, &pedal_tx_active)) {
        last_pedal_tx_time = currentTime;
    } else {
        CLEAR_BIT(huart1.Instance->CR1, USART_CR1_TE);
        SET_BIT(huart1.Instance->CR1, USART_CR1_RE);
    }
}
#endif

/* Process board responses, poll their status and send eligible outgoing messages. */
void board_uart_process(void) {
    for (unsigned budget = 0; budget < UART_RX_SLOTS && rx_tail != rx_head; ++budget) {
        board_commands_dispatch(rx_frames[rx_tail]);
        __DMB();
        rx_tail = (rx_tail + 1) % UART_RX_SLOTS;
    }
    if (diagnostic_mode || currentTime < TIMING__ALL___SERIAL_IGNORE_WINDOW_MS)
        return;
#if defined(BACCABLE_C1)
    if (runtime_state.low_consume_is_active)
        return;
    if (currentTime - runtime_state.all_processors_wakeup_time >
        TIMING__C1____DELAY_BEFORE_OTHER_CHIP_STATUS_REQUEST_MS) {
        if (currentTime - last_c2_poll_time > TIMING__C1____C2_STATUS_REQUEST_TIMEOUT_MS) {
            const uint8_t command[] = {C2BusID, C2cmdGetStatus};
            board_uart_send(command, sizeof(command));
            last_c2_poll_time = currentTime;
        }
        if (currentTime - last_bh_poll_time > TIMING__C1____BH_STATUS_REQUEST_TIMEOUT_MS) {
            const uint8_t command[] = {BhBusIDgetStatus};
            board_uart_send(command, sizeof(command));
            last_bh_poll_time = currentTime;
        }
    }
    if (currentTime - last_sent_serial_msg_time <= TIMING__C1____SERIAL_TIMEOUT_REPLY_MS)
        return;
    if (queue_start(tx_queue, &huart2, active_tx, UART_BUFFER_SIZE, &board_tx_active)) {
        last_sent_serial_msg_time = currentTime;
        switch (active_tx[0]) {
        case C2BusID:
        case C2_Bh_BusID:
        case AllResetFaults:
            last_c2_poll_time = currentTime;
            break;
        case BhBusIDgetStatus:
            last_bh_poll_time = currentTime;
            break;
        default:
            break;
        }
    }
#elif defined(BACCABLE_C2) || defined(BACCABLE_BH)
    if (currentTime - runtime_state.we_can_send_a_message_reply < TIMING__C2_BH_SERIAL_TIMEOUT_REPLY_MS)
        queue_start(tx_queue, &huart2, active_tx, UART_BUFFER_SIZE, &board_tx_active);
#endif
}
