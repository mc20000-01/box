/* WiFi driver C-side structures and internal foundation */
#ifndef BX_WIFI_H
#define BX_WIFI_H

#include <stdint.h>
#include <stddef.h>

/* WiFi environment detection */
typedef enum {
    BX_WIFI_ENV_WEB = 0,      /* Browser/WASM - no hardware access */
    BX_WIFI_ENV_NATIVE = 1,   /* Native app - host OS network stack */
    BX_WIFI_ENV_BAREMETAL = 2 /* Bare metal - direct hardware access */
} bx_wifi_env_t;

/* WiFi security types */
typedef enum {
    BX_WIFI_SEC_OPEN = 0,
    BX_WIFI_SEC_WEP = 1,
    BX_WIFI_SEC_WPA = 2,
    BX_WIFI_SEC_WPA2 = 3,
    BX_WIFI_SEC_WPA3 = 4
} bx_wifi_sec_t;

/* WiFi connection state */
typedef enum {
    BX_WIFI_DISCONNECTED = 0,
    BX_WIFI_SCANNING = 1,
    BX_WIFI_CONNECTING = 2,
    BX_WIFI_CONNECTED = 3,
    BX_WIFI_FAILED = 4
} bx_wifi_state_t;

/* Network interface info */
typedef struct {
    char name[32];
    char mac[18];
    uint8_t active;
} bx_wifi_interface_t;

/* Scan result */
typedef struct {
    char ssid[64];
    int8_t rssi;
    bx_wifi_sec_t security;
    uint8_t channel;
} bx_wifi_scan_result_t;

/* WiFi device context */
typedef struct {
    bx_wifi_env_t env;
    bx_wifi_state_t state;
    bx_wifi_interface_t current_iface;
    bx_wifi_scan_result_t *scan_results;
    uint32_t scan_count;
    uint32_t scan_capacity;
    char connected_ssid[64];
    uint32_t ip_addr;
    /* Bare metal specific */
    void *hw_priv;  /* Hardware-specific private data */
} bx_wifi_ctx_t;

/* Global WiFi context */
extern bx_wifi_ctx_t g_bx_wifi;

/* Environment detection */
bx_wifi_env_t bx_wifi_detect_env(void);

/* Initialization */
int bx_wifi_init(void);

/* Interface management */
int bx_wifi_defu(uint8_t query_only, const char *iface_name, char **out_boxes, uint32_t *out_count);

/* Scanning */
int bx_wifi_scan(const char *iface_name, bx_wifi_scan_result_t **results, uint32_t *count);

/* Connection */
int bx_wifi_creq(const char *ssid, bx_wifi_sec_t sec_type, const char *password);

/* Status */
void bx_wifi_status(char *out_box, size_t box_size);

/* Disconnect */
int bx_wifi_disconnect(void);

/* Bare metal internal functions */
int bx_wifi_hw_init(void);
int bx_wifi_hw_scan(void);
int bx_wifi_hw_connect(const char *ssid, bx_wifi_sec_t sec, const char *pwd);
void bx_wifi_hw_disconnect(void);
void bx_wifi_hw_get_status(bx_wifi_state_t *state, char *ssid, uint32_t *ip);

#endif