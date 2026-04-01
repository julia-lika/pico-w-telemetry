#include <Arduino.h>
#include "config.h"

// ── ADC moving-average state ───────────────────────────
static float   avgBuffer[MOVING_AVG_SIZE];
static int     avgIndex    = 0;
static bool    avgFull     = false;

// ── Debounce state ─────────────────────────────────────
static int     lastRaw     = HIGH;
static int     stableState = HIGH;
static unsigned long debounceT = 0;

// ── Timing ─────────────────────────────────────────────
static unsigned long lastAnalog  = 0;
static unsigned long lastDigital = 0;

// ════════════════════════════════════════════════════════
//  Analog — ADC with moving-average smoothing
// ════════════════════════════════════════════════════════

float readAnalogSmoothed() {
  int raw = analogRead(ANALOG_PIN);
  float voltage = raw * 3.3f / 4095.0f;
  float temperature = voltage * 100.0f;   // LM35-like: 10 mV/°C

  avgBuffer[avgIndex] = temperature;
  avgIndex = (avgIndex + 1) % MOVING_AVG_SIZE;
  if (avgIndex == 0) avgFull = true;

  int count = avgFull ? MOVING_AVG_SIZE : avgIndex;
  float sum = 0;
  for (int i = 0; i < count; i++) sum += avgBuffer[i];
  return sum / count;
}

// ════════════════════════════════════════════════════════
//  Digital — GPIO with debouncing
// ════════════════════════════════════════════════════════

bool readDigitalDebounced() {
  int reading = digitalRead(DIGITAL_PIN);

  if (reading != lastRaw) debounceT = millis();
  lastRaw = reading;

  if ((millis() - debounceT) > DEBOUNCE_MS) {
    stableState = reading;
  }
  return stableState == LOW;  // active-LOW (INPUT_PULLUP)
}

// ════════════════════════════════════════════════════════
//  Setup & Loop
// ════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n======= Pico Telemetry (Serial) =======");

  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  analogReadResolution(12);  // 0-4095

  for (int i = 0; i < MOVING_AVG_SIZE; i++) avgBuffer[i] = 0;
}

void loop() {
  unsigned long now = millis();

  // ── analog reading ──
  if (now - lastAnalog >= ANALOG_INTERVAL) {
    float temp = readAnalogSmoothed();
    Serial.printf("[sensor] analog  | raw_value: %.2f\n", temp);
    lastAnalog = now;
  }

  // ── digital reading ──
  if (now - lastDigital >= DIGITAL_INTERVAL) {
    bool presence = readDigitalDebounced();
    Serial.printf("[sensor] digital | presence: %s\n", presence ? "detected" : "none");
    lastDigital = now;
  }

  delay(10);
}
