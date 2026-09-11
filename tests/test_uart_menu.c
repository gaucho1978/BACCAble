#include "app/powertrain.h"
#include "diagnostics/parameter_cache.h"
#include <assert.h>
#include <stdio.h>

RuntimeState runtime_state;
PedalState pedal_state;
UART_HandleTypeDef huart1, huart2;
USART_TypeDef fake_usart1, fake_usart2;
uint32_t fake_primask;
static uint32_t now = 3000, result = HAL_OK;
static const uint8_t *active;
static uint8_t expected[UART_BUFFER_SIZE];
static unsigned transmissions;
uint32_t HAL_GetTick(void) { return now; }
void HAL_Delay(uint32_t duration) { now += duration; }
void HAL_GPIO_Init(void *port, GPIO_InitTypeDef *config) {
    (void)port;
    (void)config;
}
void HAL_NVIC_SetPriority(int a, int b, int c) {
    (void)a;
    (void)b;
    (void)c;
}
void HAL_NVIC_EnableIRQ(int irq) { (void)irq; }
uint32_t HAL_HalfDuplex_Init(UART_HandleTypeDef *uart) {
    (void)uart;
    return HAL_OK;
}
uint32_t HAL_UART_Receive_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    (void)uart;
    (void)data;
    (void)size;
    return HAL_OK;
}
uint32_t HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size) {
    (void)uart;
    assert(size == UART_BUFFER_SIZE);
    if (result != HAL_OK)
        return result;
    active = data;
    memcpy(expected, data, size);
    ++transmissions;
    return HAL_OK;
}
uint32_t HAL_UART_Transmit(UART_HandleTypeDef *uart, uint8_t *data, uint16_t size, uint32_t timeout) {
    (void)timeout;
    return HAL_UART_Transmit_IT(uart, data, size);
}
void status_led_activity(void) {}
void status_led_error(void) {}
void Error_Handler(uint16_t value) {
    (void)value;
    assert(0);
}
void board_commands_dispatch(const uint8_t *message) { (void)message; }
void parameter_cache_put(uint8_t id, float value, uint32_t when) {
    (void)id;
    (void)value;
    (void)when;
}
float native_parameter_read(uint8_t id) { return id; }
static void display(char c) {
    uint8_t frame[UART_BUFFER_SIZE];
    memset(frame, c, sizeof(frame));
    frame[0] = BhBusIDparamString;
    assert(board_uart_send(frame, sizeof(frame)));
}
static void finish(void) {
    assert(!memcmp(active, expected, sizeof(expected)));
    HAL_UART_TxCpltCallback(&huart2);
    now += 251;
    runtime_state.all_processors_wakeup_time = now; /* Keep this test independent of background polls. */
}
int main(void) {
    uart_init();
    runtime_state.all_processors_wakeup_time = now;
    const uint8_t command[] = {C2BusID, C2cmdNormalFrontBrake};
    display('A');
    assert(board_uart_send(command, sizeof(command)));
    display('B');
    fake_primask = 1;
    board_uart_process();
    assert(fake_primask == 1);
    assert(active[0] == C2BusID && active[1] == C2cmdNormalFrontBrake);
    display('C');
    assert(board_uart_send(command, sizeof(command)));
    assert(!memcmp(active, expected, sizeof(expected)));
    finish();
    board_uart_process();
    assert(active[0] == C2BusID);
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDparamString && active[1] == 'C');
    display('D');
    assert(!memcmp(active, expected, sizeof(expected)));
    finish();
    result = HAL_BUSY;
    unsigned before = transmissions;
    board_uart_process();
    assert(transmissions == before);
    display('E');
    result = HAL_OK;
    board_uart_process();
    assert(active[1] == 'E');
    finish();
    const uint8_t poll[] = {BhBusIDgetStatus};
    assert(board_uart_send(poll, sizeof(poll)));
    display('F');
    board_uart_process();
    assert(active[1] == 'F'); /* Screen goes ahead of poll. */
    display('G');
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDgetStatus); /* Continuous display updates cannot starve polls. */
    finish();
    board_uart_process();
    assert(active[0] == BhBusIDparamString && active[1] == 'G');
    finish();
    for (unsigned i = 0; i < 10; ++i)
        assert(board_uart_send(command, sizeof(command)));
    assert(!board_uart_send(command, sizeof(command)));
    assert(!board_uart_send(NULL, 0));
    puts("PASS: real UART display coalescing, command order, active buffer, HAL_BUSY and IRQ state");
}
