#ifndef CONFIG_H
#define CONFIG_H

// ── Wi-Fi ──────────────────────────────────────────────
#define WIFI_SSID        ""
#define WIFI_PASSWORD    ""

// ── Backend ────────────────────────────────────────────
#define BACKEND_URL      "http://ip:3000/telemetry"
#define DEVICE_ID        "pico-w-001"

// ── GPIO pins ──────────────────────────────────────────
#define DIGITAL_PIN      15    // Button / presence sensor
#define ANALOG_PIN       28    // ADC2  — potentiometer
#define STATUS_LED       LED_BUILTIN

// ── Timing (ms) ────────────────────────────────────────
#define ANALOG_INTERVAL  5000  // send analog reading every 5 s
#define DIGITAL_INTERVAL 2000  // send digital reading every 2 s
#define WIFI_CHECK_MS    10000 // check Wi-Fi every 10 s

// ── Retry ──────────────────────────────────────────────
#define MAX_RETRIES      3
#define RETRY_DELAY_MS   1000

// ── ADC smoothing ──────────────────────────────────────
#define MOVING_AVG_SIZE  10

// ── Debounce ───────────────────────────────────────────
#define DEBOUNCE_MS      50

// ── NTP ────────────────────────────────────────────────
#define NTP_SERVER       "pool.ntp.org"
#define GMT_OFFSET_SEC   -10800  // UTC-3 (Brasília)
#define DST_OFFSET_SEC   0

#endif
