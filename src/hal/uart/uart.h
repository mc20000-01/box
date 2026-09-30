/* UART Hardware Abstraction Layer */
#ifndef _HAL_UART_H
#define _HAL_UART_H

#include "hal/hal.h"

#if !HAL_ENABLE_UART
#error "UART not enabled. Define HAL_ENABLE_UART=1 to use."
#endif

/* ============================================================================
 * UART Types
 * ============================================================================ */
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD  = 2,
    UART_PARITY_MARK = 3,
    UART_PARITY_SPACE = 4,
} hal_uart_parity_t;

typedef enum {
    UART_STOP_1 = 0,
    UART_STOP_1_5 = 1,
    UART_STOP_2 = 2,
} hal_uart_stop_t;

typedef enum {
    UART_FLOW_NONE = 0,
    UART_FLOW_RTS_CTS = 1,
    UART_FLOW_XON_XOFF = 2,
} hal_uart_flow_t;

typedef struct {
    uint32_t baudrate;
    uint8_t  databits;       /* 5-9 */
    hal_uart_parity_t parity;
    hal_uart_stop_t stopbits;
    hal_uart_flow_t flow;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
} hal_uart_config_t;

typedef struct {
    void (*rx_callback)(void* user_data, const uint8_t* data, size_t len);
    void (*tx_callback)(void* user_data);
    void (*error_callback)(void* user_data, int error);
    void* user_data;
} hal_uart_callbacks_t;

typedef uint32_t hal_uart_handle_t;

typedef enum {
    UART_IDLE = 0,
    UART_BUSY_TX = 1,
    UART_BUSY_RX = 2,
    UART_ERROR = 3,
} hal_uart_state_t;

/* ============================================================================
 * UART API
 * ============================================================================ */
int hal_uart_init(void);
int hal_uart_deinit(void);

/* Open/close UART */
int hal_uart_open(uint32_t uart_id, const hal_uart_config_t* config, hal_uart_handle_t* handle);
int hal_uart_close(hal_uart_handle_t handle);

/* Configure */
int hal_uart_config(hal_uart_handle_t handle, const hal_uart_config_t* config);
int hal_uart_set_callbacks(hal_uart_handle_t handle, const hal_uart_callbacks_t* cb);

/* Status */
hal_uart_state_t hal_uart_get_state(hal_uart_handle_t handle);
uint32_t hal_uart_get_rx_available(hal_uart_handle_t handle);
uint32_t hal_uart_get_tx_free(hal_uart_handle_t handle);

/* Blocking I/O */
int hal_uart_read(hal_uart_handle_t handle, uint8_t* buf, size_t len, uint32_t timeout_ms);
int hal_uart_write(hal_uart_handle_t handle, const uint8_t* buf, size_t len, uint32_t timeout_ms);

/* Non-blocking I/O */
int hal_uart_read_async(hal_uart_handle_t handle, uint8_t* buf, size_t len);
int hal_uart_write_async(hal_uart_handle_t handle, const uint8_t* buf, size_t len);

/* Control */
int hal_uart_flush(hal_uart_handle_t handle);
int hal_uart_flush_rx(hal_uart_handle_t handle);
int hal_uart_flush_tx(hal_uart_handle_t handle);
int hal_uart_break(hal_uart_handle_t handle, int enable);
int hal_uart_set_baudrate(hal_uart_handle_t handle, uint32_t baudrate);

/* Get/Set modem control lines */
int hal_uart_set_rts(hal_uart_handle_t handle, int level);
int hal_uart_set_dtr(hal_uart_handle_t handle, int level);
int hal_uart_get_cts(hal_uart_handle_t handle);
int hal_uart_get_dsr(hal_uart_handle_t handle);
int hal_uart_get_ri(hal_uart_handle_t handle);
int hal_uart_get_dcd(hal_uart_handle_t handle);

/* Error handling */
int hal_uart_get_errors(hal_uart_handle_t handle);
void hal_uart_clear_errors(hal_uart_handle_t handle);

#endif /* _HAL_UART_H */
