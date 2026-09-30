/* Display Hardware Abstraction Layer */
#ifndef _HAL_DISPLAY_H
#define _HAL_DISPLAY_H

#include "hal/hal.h"

#if !HAL_ENABLE_DISPLAY
#error "Display not enabled. Define HAL_ENABLE_DISPLAY=1 to use."
#endif

/* ============================================================================
 * Display Types
 * ============================================================================ */
typedef enum {
    DISPLAY_TYPE_VGA      = 0,
    DISPLAY_TYPE_HDMI     = 1,
    DISPLAY_TYPE_LVDS     = 2,
    DISPLAY_TYPE_MIPI_DSI = 3,
    DISPLAY_TYPE_SPI      = 4,
    DISPLAY_TYPE_I2C      = 5,
    DISPLAY_TYPE_PARALLEL = 6,
    DISPLAY_TYPE_RGB      = 7,
    DISPLAY_TYPE_EINK     = 8,
    DISPLAY_TYPE_OLED     = 9,
    DISPLAY_TYPE_FRAMEBUFFER = 10,
} hal_display_type_t;

typedef enum {
    DISPLAY_FMT_RGB565  = 0,
    DISPLAY_FMT_RGB888  = 1,
    DISPLAY_FMT_BGR565  = 2,
    DISPLAY_FMT_BGR888  = 3,
    DISPLAY_FMT_ARGB8888 = 3,
    DISPLAY_FMT_RGBA8888 = 4,
    DISPLAY_FMT_XRGB8888 = 5,
    DISPLAY_FMT_MONO    = 6,
    DISPLAY_FMT_GRAY8   = 7,
} hal_display_format_t;

typedef enum {
    DISPLAY_ROTATE_0   = 0,
    DISPLAY_ROTATE_90  = 1,
    DISPLAY_ROTATE_180 = 2,
    DISPLAY_ROTATE_270 = 3,
} hal_display_rotation_t;

typedef struct {
    hal_display_type_t type;
    hal_display_format_t format;
    uint16_t width;
    uint16_t height;
    uint16_t stride;  /* bytes per line */
    hal_display_rotation_t rotation;
    uint8_t bpp;
    void* framebuffer;  /* NULL = allocate internally */
    size_t fb_size;
} hal_display_config_t;

typedef enum {
    DISPLAY_POWER_OFF = 0,
    DISPLAY_POWER_ON  = 1,
    DISPLAY_POWER_STANDBY = 2,
    DISPLAY_POWER_SUSPEND = 3,
} hal_display_power_t;

typedef struct {
    int16_t x, y;
    uint16_t w, h;
} hal_rect_t;

/* ============================================================================
 * Display API
 * ============================================================================ */
int hal_display_init(void);
int hal_display_deinit(void);

/* Open/close display */
int hal_display_open(uint32_t display_id, const hal_display_config_t* config, uint32_t* handle);
int hal_display_close(uint32_t handle);

/* Configuration */
int hal_display_config(uint32_t handle, const hal_display_config_t* config);
int hal_display_get_config(uint32_t handle, hal_display_config_t* config);

/* Power management */
int hal_display_set_power(uint32_t handle, hal_display_power_t power);
hal_display_power_t hal_display_get_power(uint32_t handle);

/* Framebuffer access */
void* hal_display_get_framebuffer(uint32_t handle);
size_t hal_display_get_fb_size(uint32_t handle);
uint32_t hal_display_get_stride(uint32_t handle);

/* Drawing primitives */
int hal_display_clear(uint32_t handle, uint32_t color);
int hal_display_draw_pixel(uint32_t handle, int16_t x, int16_t y, uint32_t color);
uint32_t hal_display_get_pixel(uint32_t handle, int16_t x, int16_t y);
int hal_display_draw_line(uint32_t handle, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);
int hal_display_draw_rect(uint32_t handle, int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t color);
int hal_display_fill_rect(uint32_t handle, int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t color);
int hal_display_draw_circle(uint32_t handle, int16_t x, int16_t y, uint16_t r, uint32_t color);
int hal_display_fill_circle(uint32_t handle, int16_t x, int16_t y, uint16_t r, uint32_t color);

/* Text rendering */
int hal_display_draw_char(uint32_t handle, int16_t x, int16_t y, char c, uint32_t fg, uint32_t bg);
int hal_display_draw_string(uint32_t handle, int16_t x, int16_t y, const char* str, uint32_t fg, uint32_t bg);
int hal_display_draw_string_utf8(uint32_t handle, int16_t x, int16_t y, const char* str, uint32_t fg, uint32_t bg);
int hal_display_set_font(uint32_t handle, const void* font_data, uint16_t char_w, uint16_t char_h, uint8_t first_char, uint8_t last_char);

/* Bitmap operations */
int hal_display_draw_bitmap(uint32_t handle, int16_t x, int16_t y, uint16_t w, uint16_t h, const void* data, hal_display_format_t fmt);
int hal_display_draw_bitmap_alpha(uint32_t handle, int16_t x, int16_t y, uint16_t w, uint16_t h, const void* data, const void* alpha);

/* Display control */
int hal_display_flush(uint32_t handle);
int hal_display_set_rotation(uint32_t handle, hal_display_rotation_t rot);
int hal_display_set_brightness(uint32_t handle, uint8_t brightness);
uint8_t hal_display_get_brightness(uint32_t handle);
int hal_display_set_contrast(uint32_t handle, uint8_t contrast);
int hal_display_set_palette(uint32_t handle, const uint32_t* palette, uint16_t start, uint16_t len);
int hal_display_invert(uint32_t handle, int invert);

/* Window/Region operations */
int hal_display_set_window(uint32_t handle, int16_t x, int16_t y, uint16_t w, uint16_t h);
int hal_display_scroll(uint32_t handle, int16_t dx, int16_t dy);
int hal_display_copy_region(uint32_t handle, int16_t src_x, int16_t src_y, int16_t dst_x, int16_t dst_y, uint16_t w, uint16_t h);

/* Double buffering */
int hal_display_swap_buffers(uint32_t handle);
int hal_display_set_vsync(uint32_t handle, int enable);

/* Info */
int hal_display_get_info(uint32_t handle, hal_display_config_t* config);
const char* hal_display_get_name(uint32_t handle);

#endif /* _HAL_DISPLAY_H */
