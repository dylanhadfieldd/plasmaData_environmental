#include <Arduino.h>

#include "Config.h"
#include "Numeric.h"
#include "SensorManager.h"
#include "TrendBuffer.h"
#include "UiRenderer.h"

namespace {

SensorManager gSensors;
TrendBuffer gTrends;
UiRenderer gUi;

uint32_t gLastTrendMs = 0;
uint32_t gLastUiMs = 0;
uint32_t gLastSerialMs = 0;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println(F("=============================================="));
  Serial.println(F("ESP32 Sensor Dashboard"));
  Serial.println(F("=============================================="));

  const bool sensorsReady = gSensors.begin();
  if (!sensorsReady) {
    Serial.println(F("[BOOT] Warning: no sensors came online during startup."));
  }

  gUi.begin();

  const uint32_t nowMs = millis();
  gLastTrendMs = nowMs;
  gLastUiMs = nowMs;
  gLastSerialMs = nowMs;
}

void loop() {
  const uint32_t nowMs = millis();

  gUi.pollInput(nowMs);
  gSensors.update(nowMs);
  const SensorSnapshot& snapshot = gSensors.snapshot();

  if (nowMs - gLastTrendMs >= cfg::kTrendSampleMs) {
    if (app::isFiniteNumber(snapshot.temperatureC) || app::isFiniteNumber(snapshot.humidityPct) ||
        app::isFiniteNumber(snapshot.distanceMm)) {
      gTrends.push(snapshot.temperatureC, snapshot.humidityPct, snapshot.distanceMm, nowMs);
    }
    gLastTrendMs = nowMs;
  }

  if (nowMs - gLastUiMs >= cfg::kUiFrameMs) {
    gUi.render(nowMs, snapshot, gTrends);
    gLastUiMs = nowMs;
  }

  if (nowMs - gLastSerialMs >= cfg::kSerialLogMs) {
    Serial.printf("[DATA] T=%.2fC RH=%.2f%% Dist=%.1fmm | TH=%s Dist=%s\n", snapshot.temperatureC,
                  snapshot.humidityPct, snapshot.distanceMm,
                  snapshot.tempHumidityOk ? "ok" : "offline",
                  snapshot.distanceOk ? "ok" : "offline");
    gLastSerialMs = nowMs;
  }
}
