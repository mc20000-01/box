/* GPIO Hardware Abstraction Layer */
#ifndef _HAL_GPIO_H
#define _HAL_GPIO_H

#include "hal/hal.h"

#if !HAL_ENABLE_GPIO
#error "GPIO not enabled. Define HAL_ENABLE_GPIO=1 to use."
#endif

/* ============================================================================
 * GPIO Types
 * ============================================================================ */
typedef enum {
    GPIO_DIR_INPUT  = 0,
    GPIO_DIR_OUTPUT = 1,
    GPIO_DIR_ALT    = 2,  /* Alternate function */
    GPIO_DIR_ANALOG = 3,  /* Analog mode */
} hal_gpio_dir_t;

typedef enum {
    GPIO_PULL_NONE  = 0,
    GPIO_PULL_UP    = 1,
    GPIO_PULL_DOWN  = 2,
    GPIO_PULL_KEEP  = 3,
} hal_gpio_pull_t;

typedef enum {
    GPIO_DRIVE_LOW    = 0,  /* 2mA */
    GPIO_DRIVE_MED    = 1,  /* 4mA */
    GPIO_DRIVE_HIGH   = 2,  /* 8mA */
    GPIO_DRIVE_VHIGH  = 3,  /* 12mA+ */
} hal_gpio_drive_t;

typedef enum {
    GPIO_EDGE_NONE = 0,
    GPIO_EDGE_RISING = 1,
    GPIO_EDGE_FALLING = 2,
    GPIO_EDGE_BOTH = 3,
} hal_gpio_edge_t;

typedef enum {
    GPIO_LEVEL_LOW  = 0,
    GPIO_LEVEL_HIGH = 1,
} hal_gpio_level_t;

typedef struct {
    uint32_t pin;
    hal_gpio_dir_t dir;
    hal_gpio_pull_t pull;
    hal_gpio_drive_t drive;
    hal_gpio_level_t initial;
} hal_gpio_config_t;

typedef void (*hal_gpio_isr_t)(uint32_t pin, void* user_data);

/* ============================================================================
 * GPIO API
 * ============================================================================ */

/* Initialize GPIO subsystem */
int hal_gpio_init(void);
int hal_gpio_deinit(void);

/* Configure a pin */
int hal_gpio_config(uint32_t pin, const hal_gpio_config_t* config);

/* Set pin direction */
int hal_gpio_set_dir(uint32_t pin, hal_gpio_dir_t dir);

/* Set pull resistor */
int hal_gpio_set_pull(uint32_t pin, hal_gpio_pull_t pull);

/* Set drive strength */
int hal_gpio_set_drive(uint32_t pin, hal_gpio_drive_t drive);

/* Read pin level */
hal_gpio_level_t hal_gpio_read(uint32_t pin);

/* Write pin level */
int hal_gpio_write(uint32_t pin, hal_gpio_level_t level);

/* Toggle pin */
int hal_gpio_toggle(uint32_t pin);

/* Set interrupt handler */
int hal_gpio_set_isr(uint32_t pin, hal_gpio_edge_t edge, hal_gpio_isr_t isr, void* user_data);

/* Enable/disable interrupt */
int hal_gpio_enable_irq(uint32_t pin);
int hal_gpio_disable_irq(uint32_t pin);

/* Get/clear interrupt status */
int hal_gpio_get_irq_status(uint32_t pin);
int hal_gpio_clear_irq_status(uint32_t pin);

/* Lock/unlock pin configuration (for secure pins) */
int hal_gpio_lock(uint32_t pin);
int hal_gpio_unlock(uint32_t pin);

/* ============================================================================
 * Port/Group Operations (for bulk operations)
 * ============================================================================ */
typedef uint32_t hal_gpio_port_t;
typedef uint32_t hal_gpio_mask_t;

int hal_gpio_port_read(hal_gpio_port_t port, hal_gpio_mask_t* value);
int hal_gpio_port_write(hal_gpio_port_t port, hal_gpio_mask_t value);
int hal_gpio_port_set_dir(hal_gpio_port_t port, hal_gpio_mask_t mask, hal_gpio_dir_t dir);

/* ============================================================================
 * Platform-specific pin mapping (implemented per platform)
 * ============================================================================ */
uint32_t hal_gpio_pin_from_name(const char* name);
const char* hal_gpio_name_from_pin(uint32_t pin);
int hal_gpio_get_port_pin(uint32_t pin, hal_gpio_port_t* port, uint32_t* pin_num);

#endif /* _HAL_GPIO_H */
