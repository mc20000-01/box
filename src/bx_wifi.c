/* WiFi driver C-side implementation */
#define _POSIX_C_SOURCE 200809L
#include "bx_wifi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bx_wifi_ctx_t g_bx_wifi = {0};

/* Environment detection */
bx_wifi_env_t bx_wifi_detect_env(void) {
#if defined(BX_WASM) || defined(__EMSCRIPTEN__)
    return BX_WIFI_ENV_WEB;
#elif defined(BX_FREESTANDING) || defined(BX_MULTIBOOT) || defined(BX_UEFI)
    return BX_WIFI_ENV_BAREMETAL;
#else
    return BX_WIFI_ENV_NATIVE;
#endif
}

/* Initialize WiFi subsystem */
int bx_wifi_init(void) {
    g_bx_wifi.env = bx_wifi_detect_env();
    g_bx_wifi.state = BX_WIFI_DISCONNECTED;
    g_bx_wifi.scan_results = NULL;
    g_bx_wifi.scan_count = 0;
    g_bx_wifi.scan_capacity = 0;
    g_bx_wifi.connected_ssid[0] = '\0';
    g_bx_wifi.ip_addr = 0;
    g_bx_wifi.hw_priv = NULL;

    if (g_bx_wifi.env == BX_WIFI_ENV_BAREMETAL) {
        return bx_wifi_hw_init();
    }
    return 0;
}

/* Interface enumeration */
int bx_wifi_defu(uint8_t query_only, const char *iface_name, char **out_boxes, uint32_t *out_count) {
    if (g_bx_wifi.env == BX_WIFI_ENV_WEB) {
        return -1; /* Not supported in web */
    }

    if (query_only) {
        /* Return list of interfaces */
        static const char *native_ifaces[] = {
            "wlan0", "wlan1", "wlp3s0", "wlp4s0", "wifi0"
        };
        uint32_t count = sizeof(native_ifaces) / sizeof(native_ifaces[0]);
        
        if (g_bx_wifi.env == BX_WIFI_ENV_BAREMETAL) {
            count = 1;
            native_ifaces[0] = "wlan0";
        }
        
        if (out_boxes) {
            for (uint32_t i = 0; i < count; i++) {
                out_boxes[i] = strdup(native_ifaces[i]);
            }
        }
        if (out_count) *out_count = count;
        return 0;
    } else {
        /* Set default interface */
        if (!iface_name) return -1;
        strncpy(g_bx_wifi.current_iface.name, iface_name, sizeof(g_bx_wifi.current_iface.name) - 1);
        g_bx_wifi.current_iface.active = 1;
        return 0;
    }
}

/* Scanning */
int bx_wifi_scan(const char *iface_name, bx_wifi_scan_result_t **results, uint32_t *count) {
    if (g_bx_wifi.env == BX_WIFI_ENV_WEB) {
        return -1;
    }

    g_bx_wifi.state = BX_WIFI_SCANNING;

    if (g_bx_wifi.env == BX_WIFI_ENV_BAREMETAL) {
        return bx_wifi_hw_scan();
    }

    /* Native simulation - return fake results */
    static bx_wifi_scan_result_t native_results[] = {
        {"HomeNetwork", -45, BX_WIFI_SEC_WPA2, 6},
        {"OfficeWiFi", -62, BX_WIFI_SEC_WPA2, 11},
        {"GuestNetwork", -70, BX_WIFI_SEC_OPEN, 1},
        {"CoffeeShop", -55, BX_WIFI_SEC_WPA3, 36},
        {"Neighbor5G", -68, BX_WIFI_SEC_WPA2, 149}
    };
    uint32_t n = sizeof(native_results) / sizeof(native_results[0]);

    if (g_bx_wifi.scan_capacity < n) {
        g_bx_wifi.scan_results = realloc(g_bx_wifi.scan_results, n * sizeof(bx_wifi_scan_result_t));
        g_bx_wifi.scan_capacity = n;
    }
    memcpy(g_bx_wifi.scan_results, native_results, n * sizeof(bx_wifi_scan_result_t));
    g_bx_wifi.scan_count = n;

    if (results) *results = g_bx_wifi.scan_results;
    if (count) *count = n;
    g_bx_wifi.state = BX_WIFI_DISCONNECTED;
    return 0;
}

/* Connection request */
int bx_wifi_creq(const char *ssid, bx_wifi_sec_t sec_type, const char *password) {
    if (!ssid || g_bx_wifi.env == BX_WIFI_ENV_WEB) {
        return -1;
    }

    g_bx_wifi.state = BX_WIFI_CONNECTING;

    int rc = -1;
    if (g_bx_wifi.env == BX_WIFI_ENV_BAREMETAL) {
        rc = bx_wifi_hw_connect(ssid, sec_type, password);
    } else {
        /* Native simulation */
        rc = 0; /* Simulate success */
    }

    if (rc == 0) {
        g_bx_wifi.state = BX_WIFI_CONNECTED;
        strncpy(g_bx_wifi.connected_ssid, ssid, sizeof(g_bx_wifi.connected_ssid) - 1);
        g_bx_wifi.ip_addr = 0xC0A80164; /* 192.168.1.100 */
    } else {
        g_bx_wifi.state = BX_WIFI_FAILED;
    }
    return rc;
}

/* Status query */
void bx_wifi_status(char *out_box, size_t box_size) {
    const char *state_str[] = {
        "DISCONNECTED", "SCANNING", "CONNECTING", "CONNECTED", "FAILED"
    };
    const char *env_str[] = {
        "WEB", "NATIVE", "BAREMETAL"
    };

    snprintf(out_box, box_size,
        "state=%s env=%s ssid=%s ip=%u.%u.%u.%u",
        state_str[g_bx_wifi.state],
        env_str[g_bx_wifi.env],
        g_bx_wifi.connected_ssid[0] ? g_bx_wifi.connected_ssid : "none",
        (g_bx_wifi.ip_addr >> 24) & 0xFF,
        (g_bx_wifi.ip_addr >> 16) & 0xFF,
        (g_bx_wifi.ip_addr >> 8) & 0xFF,
        g_bx_wifi.ip_addr & 0xFF
    );
}

/* Disconnect */
int bx_wifi_disconnect(void) {
    if (g_bx_wifi.env == BX_WIFI_ENV_WEB) {
        return -1;
    }

    if (g_bx_wifi.env == BX_WIFI_ENV_BAREMETAL) {
        bx_wifi_hw_disconnect();
    }

    g_bx_wifi.state = BX_WIFI_DISCONNECTED;
    g_bx_wifi.connected_ssid[0] = '\0';
    g_bx_wifi.ip_addr = 0;
    return 0;
}

/* Bare metal hardware stubs */
int bx_wifi_hw_init(void) {
    /* TODO: PCI enumeration, MMIO mapping, firmware loading */
    return 0;
}

int bx_wifi_hw_scan(void) {
    /* TODO: Trigger hardware scan, parse beacon frames */
    return 0;
}

int bx_wifi_hw_connect(const char *ssid, bx_wifi_sec_t sec, const char *pwd) {
    /* TODO: Auth/Assoc state machine, WPA2/WPA3 handshake */
    return 0;
}

void bx_wifi_hw_disconnect(void) {
    /* TODO: Send deauth frame */
}

void bx_wifi_hw_get_status(bx_wifi_state_t *state, char *ssid, uint32_t *ip) {
    /* TODO: Query hardware state */
}