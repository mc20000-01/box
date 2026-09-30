/* BoxedLANG Hardware Abstraction Layer - Core Definitions
 * Modular, configurable HAL for bare-metal on any CPU
 * Only includes what's needed per hardware device
 */

#ifndef _HAL_H
#define _HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Configuration - Enable only what you need per device
 * Define these in your board config or via compiler flags
 * ============================================================================ */

/* Hardware modules - define to 1 to enable */
#ifndef HAL_ENABLE_GPIO
#define HAL_ENABLE_GPIO 1
#endif

#ifndef HAL_ENABLE_I2C
#define HAL_ENABLE_I2C 0
#endif

#ifndef HAL_ENABLE_SPI
#define HAL_ENABLE_SPI 0
#endif

#ifndef HAL_ENABLE_UART
#define HAL_ENABLE_UART 1
#endif

#ifndef HAL_ENABLE_WIFI
#define HAL_ENABLE_WIFI 0
#endif

#ifndef HAL_ENABLE_DISPLAY
#define HAL_ENABLE_DISPLAY 1
#endif

#ifndef HAL_ENABLE_STORAGE
#define HAL_ENABLE_STORAGE 0
#endif

#ifndef HAL_ENABLE_USB
#define HAL_ENABLE_USB 0
#endif

#ifndef HAL_ENABLE_TIMER
#define HAL_ENABLE_TIMER 0
#endif

#ifndef HAL_ENABLE_INTERRUPT
#define HAL_ENABLE_INTERRUPT 0
#endif

#ifndef HAL_ENABLE_DMA
#define HAL_ENABLE_DMA 0
#endif

#ifndef HAL_ENABLE_POWER
#define HAL_ENABLE_POWER 0
#endif

#ifndef HAL_ENABLE_MEMORY
#define HAL_ENABLE_MEMORY 0
#endif

#ifndef HAL_ENABLE_NETWORK
#define HAL_ENABLE_NETWORK 0
#endif

#ifndef HAL_ENABLE_USB_HOST
#define HAL_ENABLE_USB_HOST 0
#endif

#ifndef HAL_ENABLE_USB_DEVICE
#define HAL_ENABLE_USB_DEVICE 0
#endif

#ifndef HAL_ENABLE_AUDIO
#define HAL_ENABLE_AUDIO 0
#endif

#ifndef HAL_ENABLE_VIDEO
#define HAL_ENABLE_VIDEO 0
#endif

#ifndef HAL_ENABLE_CRYPTO
#define HAL_ENABLE_CRYPTO 0
#endif

#ifndef HAL_ENABLE_RTC
#define HAL_ENABLE_RTC 0
#endif

#ifndef HAL_ENABLE_ADC
#define HAL_ENABLE_ADC 0
#endif

#ifndef HAL_ENABLE_DAC
#define HAL_ENABLE_DAC 0
#endif

#ifndef HAL_ENABLE_PWM
#define HAL_ENABLE_PWM 0
#endif

#ifndef HAL_ENABLE_WATCHDOG
#define HAL_ENABLE_WATCHDOG 0
#endif

/* ============================================================================
 * CPU Architecture - auto-detected or override
 * ============================================================================ */
#if defined(__x86_64__) || defined(__amd64__)
#define HAL_CPU_X86_64 1
#define HAL_CPU_BITS 64
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__i386__) || defined(__i686__)
#define HAL_CPU_X86 1
#define HAL_CPU_BITS 32
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__aarch64__)
#define HAL_CPU_ARM64 1
#define HAL_CPU_BITS 64
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__arm__) || defined(__ARM__)
#define HAL_CPU_ARM 1
#define HAL_CPU_BITS 32
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__riscv) && (__riscv_xlen == 64)
#define HAL_CPU_RISCV64 1
#define HAL_CPU_BITS 64
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__riscv) && (__riscv_xlen == 32)
#define HAL_CPU_RISCV32 1
#define HAL_CPU_BITS 32
#define HAL_CPU_ENDIAN_LITTLE 1
#elif defined(__mips__) && (_MIPS_SZLONG == 64)
#define HAL_CPU_MIPS64 1
#define HAL_CPU_BITS 64
#elif defined(__mips__)
#define HAL_CPU_MIPS 1
#define HAL_CPU_BITS 32
#elif defined(__powerpc64__)
#define HAL_CPU_PPC64 1
#define HAL_CPU_BITS 64
#elif defined(__powerpc__)
#define HAL_CPU_PPC 1
#define HAL_CPU_BITS 32
#elif defined(__m68k__)
#define HAL_CPU_M68K 1
#define HAL_CPU_BITS 32
#elif defined(__xtensa__)
#define HAL_CPU_XTENSA 1
#define HAL_CPU_BITS 32
#elif defined(__avr__)
#define HAL_CPU_AVR 1
#define HAL_CPU_BITS 8
#elif defined(__msp430__)
#define HAL_CPU_MSP430 1
#define HAL_CPU_BITS 16
#else
#define HAL_CPU_UNKNOWN 1
#define HAL_CPU_BITS 32
#endif

/* ============================================================================
 * Common Types
 * ============================================================================ */
typedef int8_t  hal_i8;
typedef int16_t hal_i16;
typedef int32_t hal_i32;
typedef int64_t hal_i64;
typedef uint8_t  hal_u8;
typedef uint16_t hal_u16;
typedef uint32_t hal_u32;
typedef uint64_t hal_u64;
typedef float hal_f32;
typedef double hal_f64;
typedef size_t hal_size;
typedef intptr_t hal_intptr;
typedef uintptr_t hal_uintptr;

/* Result codes */
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = -1,
    HAL_ERROR_NOT_IMPLEMENTED = -2,
    HAL_ERROR_INVALID_PARAM = -3,
    HAL_ERROR_NOT_INITIALIZED = -4,
    HAL_ERROR_BUSY = -5,
    HAL_ERROR_TIMEOUT = -6,
    HAL_ERROR_NO_MEMORY = -7,
    HAL_ERROR_HARDWARE_FAULT = -8,
} hal_result_t;

/* Function pointer types */
typedef void (*hal_isr_t)(void*);
typedef void (*hal_callback_t)(void*, void*);

/* ============================================================================
 * Module Interface - each HAL module implements this
 * ============================================================================ */
typedef struct hal_module {
    const char* name;
    int (*init)(void);
    int (*deinit)(void);
    int (*suspend)(void);
    int (*resume)(void);
    const char* version;
} hal_module_t;

/* ============================================================================
 * Conditional Module Includes
 * ============================================================================ */
#if HAL_ENABLE_GPIO
#include "hal/gpio/gpio.h"
#endif

#if HAL_ENABLE_I2C
#include "hal/i2c/i2c.h"
#endif

#if HAL_ENABLE_SPI
#include "hal/spi/spi.h"
#endif

#if HAL_ENABLE_UART
#include "hal/uart/uart.h"
#endif

#if HAL_ENABLE_WIFI
#include "hal/wifi/wifi.h"
#endif

#if HAL_ENABLE_DISPLAY
#include "hal/display/display.h"
#endif

#if HAL_ENABLE_STORAGE
#include "hal/storage/storage.h"
#endif

#if HAL_ENABLE_USB
#include "hal/usb/usb.h"
#endif

#if HAL_ENABLE_TIMER
#include "hal/timer/timer.h"
#endif

#if HAL_ENABLE_INTERRUPT
#include "hal/interrupt/interrupt.h"
#endif

#if HAL_ENABLE_DMA
#include "hal/dma/dma.h"
#endif

#if HAL_ENABLE_POWER
#include "hal/power/power.h"
#endif

#if HAL_ENABLE_MEMORY
#include "hal/memory/memory.h"
#endif

#if HAL_ENABLE_NETWORK
#include "hal/network/network.h"
#endif

#if HAL_ENABLE_USB_HOST
#include "hal/usb_host/usb_host.h"
#endif

#if HAL_ENABLE_USB_DEVICE
#include "hal/usb_device/usb_device.h"
#endif

#if HAL_ENABLE_AUDIO
#include "hal/audio/audio.h"
#endif

#if HAL_ENABLE_VIDEO
#include "hal/video/video.h"
#endif

#if HAL_ENABLE_CRYPTO
#include "hal/crypto/crypto.h"
#endif

#if HAL_ENABLE_RTC
#include "hal/rtc/rtc.h"
#endif

#if HAL_ENABLE_ADC
#include "hal/adc/adc.h"
#endif

#if HAL_ENABLE_DAC
#include "hal/dac/dac.h"
#endif

#if HAL_ENABLE_PWM
#include "hal/pwm/pwm.h"
#endif

#if HAL_ENABLE_WATCHDOG
#include "hal/watchdog/watchdog.h"
#endif

/* Only include module headers whose header file actually exists in this
 * build tree, so the HAL stays standalone-compilable until a module has
 * both a header and an implementation checked in. */
#if defined(__has_include)
#ifndef HAL_INCLUDE_MODULE
#define HAL_INCLUDE_MODULE(flag, path) (flag && __has_include(path))
#endif
#else
#define HAL_INCLUDE_MODULE(flag, path) flag
#endif

/* ============================================================================
 * HAL Initialization
 * ============================================================================ */
int hal_init(void);
int hal_deinit(void);
const char* hal_get_version(void);
const char* hal_get_cpu_name(void);
uint32_t hal_get_cpu_features(void);

/* ============================================================================
 * Compiler/Platform Helpers
 * ============================================================================ */
#define HAL_UNUSED(x) (void)(x)
#define HAL_FORCE_INLINE __attribute__((always_inline)) inline
#define HAL_NORETURN __attribute__((noreturn))
#define HAL_WEAK __attribute__((weak))
#define HAL_SECTION(x) __attribute__((section(x)))
#define HAL_ALIGNED(x) __attribute__((aligned(x)))
#define HAL_PACKED __attribute__((packed))

/* Memory barriers */
#define HAL_DMB() __asm__ __volatile__ ("dmb" ::: "memory")
#define HAL_DSB() __asm__ __volatile__ ("dsb" ::: "memory")
#define HAL_ISB() __asm__ __volatile__ ("isb" ::: "memory")

/* Atomic operations */
#define HAL_ATOMIC_LOAD(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define HAL_ATOMIC_STORE(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define HAL_ATOMIC_ADD(ptr, val) __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
#define HAL_ATOMIC_CAS(ptr, expected, desired) __atomic_compare_exchange_n(ptr, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif /* _HAL_H */
