#ifndef CONFIG_H
#define CONFIG_H

// ── GPIO pins ──────────────────────────────────────────
#define DIGITAL_PIN      15    // Button / presence sensor
#define ANALOG_PIN       28    // ADC2  — potentiometer
#define STATUS_LED       LED_BUILTIN

// ── Timing (ms) ────────────────────────────────────────
#define ANALOG_INTERVAL  5000  // print analog reading every 5 s
#define DIGITAL_INTERVAL 2000  // print digital reading every 2 s

// ── ADC smoothing ──────────────────────────────────────
#define MOVING_AVG_SIZE  10

// ── Debounce ───────────────────────────────────────────
#define DEBOUNCE_MS      50

#endif
