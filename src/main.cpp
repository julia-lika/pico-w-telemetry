#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
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
static unsigned long lastWiFiChk = 0;

// ════════════════════════════════════════════════════════
//  Wi-Fi
// ════════════════════════════════════════════════════════

void connectWiFi() {
  Serial.printf("[wifi] Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] Connected — IP %s\n", WiFi.localIP().toString().c_str());
    digitalWrite(STATUS_LED, HIGH);
  } else {
    Serial.println("[wifi] Connection failed. Will retry later.");
    digitalWrite(STATUS_LED, LOW);
  }
}

void ensureWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] Disconnected — reconnecting...");
    digitalWrite(STATUS_LED, LOW);
    connectWiFi();
  }
}

// ════════════════════════════════════════════════════════
//  NTP timestamp
// ════════════════════════════════════════════════════════

String isoTimestamp() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "1970-01-01T00:00:00";
  char buf[20];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ti);
  return String(buf);
}

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
//  HTTP telemetry sender with retry
// ════════════════════════════════════════════════════════

bool sendTelemetry(const char* sensorType, const char* nature, float value) {
  ensureWiFi();
  if (WiFi.status() != WL_CONNECTED) return false;

  JsonDocument doc;
  doc["device_id"]   = DEVICE_ID;
  doc["timestamp"]   = isoTimestamp();
  doc["sensor_type"] = sensorType;
  doc["nature"]      = nature;
  doc["value"]       = value;

  String payload;
  serializeJson(doc, payload);
  Serial.printf("[http] Sending: %s\n", payload.c_str());

  for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
    HTTPClient http;
    http.begin(BACKEND_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    int code = http.POST(payload);
    String body = http.getString();
    http.end();

    if (code == 202) {
      Serial.printf("[http] 202 Accepted — %s\n", body.c_str());
      return true;
    }

    Serial.printf("[http] Attempt %d/%d failed (HTTP %d)\n", attempt, MAX_RETRIES, code);
    if (attempt < MAX_RETRIES) delay(RETRY_DELAY_MS * attempt);
  }

  Serial.println("[http] All retries exhausted.");
  return false;
}

// ════════════════════════════════════════════════════════
//  Setup & Loop
// ════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n======= Pico W Telemetry Firmware =======");

  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  analogReadResolution(12);  // 0-4095

  for (int i = 0; i < MOVING_AVG_SIZE; i++) avgBuffer[i] = 0;

  connectWiFi();
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  Serial.println("[ntp] Time sync requested");
}

void loop() {
  unsigned long now = millis();

  // periodic Wi-Fi health check
  if (now - lastWiFiChk >= WIFI_CHECK_MS) {
    ensureWiFi();
    lastWiFiChk = now;
  }

  // ── analog reading (temperature) ──
  if (now - lastAnalog >= ANALOG_INTERVAL) {
    float temp = readAnalogSmoothed();
    Serial.printf("[sensor] Temperature (smoothed): %.2f °C\n", temp);
    sendTelemetry("temperature", "analog", temp);
    lastAnalog = now;
  }

  // ── digital reading (presence) ──
  if (now - lastDigital >= DIGITAL_INTERVAL) {
    bool presence = readDigitalDebounced();
    Serial.printf("[sensor] Presence: %s\n", presence ? "detected" : "none");
    sendTelemetry("presence", "discrete", presence ? 1.0f : 0.0f);
    lastDigital = now;
  }

  delay(10);
}
