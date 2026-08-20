#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

#include "config.h"
#include "display.h"
#include "effects.h"
#include "schedule.h"
#include "time_utils.h"

namespace {

SegmentDisplay display;
ScheduleManager schedule;

enum class ManualMode { Auto, LedScan, SingleLed, Glyph, Smile, Off };
ManualMode mode = ManualMode::Auto;
uint16_t manualLed = 0;
char manualGlyph = 'B';
uint16_t lastScannedLed = 0xFFFF;

bool clockReady = false;
bool otaReady = false;
String lastRefreshDate;
uint32_t lastScheduleAttemptMs = 0;
uint32_t lastClockAttemptMs = 0;
uint32_t funStartedAt = 0;
uint32_t funEndsAt = 0;
uint32_t nextFunAt = 0;

uint32_t randomFunDelay() {
  const uint32_t span = config::kFunAnimationMaxIntervalMs -
                        config::kFunAnimationMinIntervalMs;
  return config::kFunAnimationMinIntervalMs + random(span + 1);
}

void scheduleNextFun(uint32_t nowMs) { nextFunAt = nowMs + randomFunDelay(); }

void connectWifi() {
  if (strlen(config::kWifiSsid) == 0) {
    Serial.println("[wifi] No credentials configured; running offline/manual");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.hostname(config::kHostname);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(config::kWifiSsid, config::kWifiPassword);

  Serial.printf("[wifi] Connecting to %s", config::kWifiSsid);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < config::kWifiConnectTimeoutMs) {
    Serial.print('.');
    delay(250);
    yield();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] Connected: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[wifi] Timed out; cached schedule remains usable");
  }
}

void setupOta() {
  if (WiFi.status() != WL_CONNECTED || strlen(config::kOtaPassword) == 0) return;

  ArduinoOTA.setHostname(config::kHostname);
  ArduinoOTA.setPassword(config::kOtaPassword);
  ArduinoOTA.onStart([]() {
    display.clear(true);
    Serial.println("[ota] Update starting");
  });
  ArduinoOTA.onEnd([]() { Serial.println("\n[ota] Update complete"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[ota] %u%%\r", (progress * 100U) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[ota] Error %u\n", error);
  });
  ArduinoOTA.begin();
  otaReady = true;
  Serial.println("[ota] Ready at dragon-light.local");
}

bool findTransition(const tm& now, uint32_t& secondsLeft) {
  const int nowSeconds = now.tm_hour * 3600 + now.tm_min * 60 + now.tm_sec;
  for (const auto& window : config::kTransitions) {
    const int start = window.startHour * 3600 + window.startMinute * 60;
    const int end = window.endHour * 3600 + window.endMinute * 60;
    if (nowSeconds >= start && nowSeconds < end) {
      secondsLeft = end - nowSeconds;
      return true;
    }
  }
  return false;
}

void maybeRefreshSchedule(const tm& now) {
  if (WiFi.status() != WL_CONNECTED) return;

  const int refreshMin = config::kScheduleRefreshHour * 60 +
                         config::kScheduleRefreshMinute;
  const String today = isoDate(now);
  if (minutesSinceMidnight(now) < refreshMin || lastRefreshDate == today) return;

  constexpr uint32_t kRetryIntervalMs = 15UL * 60UL * 1000UL;
  if (lastScheduleAttemptMs != 0 &&
      millis() - lastScheduleAttemptMs < kRetryIntervalMs) return;

  lastScheduleAttemptMs = millis();
  if (schedule.refreshFromNetwork()) lastRefreshDate = today;
}

void serviceNetwork() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!clockReady &&
      (lastClockAttemptMs == 0 || millis() - lastClockAttemptMs > 60000UL)) {
    lastClockAttemptMs = millis();
    clockReady = syncClock();
  }

  if (!otaReady && strlen(config::kOtaPassword) > 0) setupOta();
}

void renderAuto(uint32_t nowMs) {
  tm now{};
  if (!getDenverTime(now)) {
    display.clear(true);
    return;
  }

  maybeRefreshSchedule(now);
  const int nowMinutes = minutesSinceMidnight(now);

  if (isWeekend(now) ||
      isBefore(nowMinutes, config::kWakeHour, config::kWakeMinute) ||
      isAtOrAfter(nowMinutes, config::kSleepHour, config::kSleepMinute)) {
    nextFunAt = 0;
    funEndsAt = 0;
    display.clear(true);
    return;
  }

  const char rotation = schedule.rotationForDate(isoDate(now));
  if (rotation == '-') {
    display.clear(true);
    return;
  }

  if (isAtOrAfter(nowMinutes, config::kCelebrationHour,
                  config::kCelebrationMinute)) {
    renderSmile(display, nowMs);
    return;
  }

  if (rotation == '\0') {
    renderUnknown(display, nowMs);
    return;
  }

  uint32_t secondsLeft = 0;
  if (findTransition(now, secondsLeft)) {
    const uint32_t roundedMinutes = (secondsLeft + 59) / 60;
    const uint8_t minutesLeft =
        static_cast<uint8_t>(roundedMinutes > 9 ? 9 : roundedMinutes);
    renderCountdown(display, rotation, minutesLeft < 1 ? 1 : minutesLeft,
                    secondsLeft, nowMs);
    return;
  }

  if (funEndsAt != 0 && static_cast<int32_t>(funEndsAt - nowMs) > 0) {
    renderFunSweep(display, rotation, nowMs, funStartedAt);
    return;
  }

  if (nextFunAt == 0) scheduleNextFun(nowMs);
  if (static_cast<int32_t>(nowMs - nextFunAt) >= 0) {
    funStartedAt = nowMs;
    funEndsAt = nowMs + config::kFunAnimationDurationMs;
    scheduleNextFun(funEndsAt);
    renderFunSweep(display, rotation, nowMs, funStartedAt);
    return;
  }

  renderDayLetter(display, rotation, nowMs);
}

void printStatus() {
  tm now{};
  if (!getDenverTime(now)) {
    Serial.println("[status] Clock unavailable");
    return;
  }
  const String today = isoDate(now);
  const char rotation = schedule.rotationForDate(today);
  Serial.printf("[status] %s %02d:%02d:%02d | rotation=%c | wifi=%s | ota=%s\n",
                today.c_str(), now.tm_hour, now.tm_min, now.tm_sec,
                rotation ? rotation : '?',
                WiFi.status() == WL_CONNECTED ? "up" : "down",
                otaReady ? "ready" : "off");
}

void printHelp() {
  Serial.println("Commands: auto | scan | led N | glyph X | smile | off | sync | status | help");
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) return;

  if (command == "auto") {
    mode = ManualMode::Auto;
  } else if (command == "scan") {
    mode = ManualMode::LedScan;
    lastScannedLed = 0xFFFF;
  } else if (command.startsWith("led ")) {
    manualLed = static_cast<uint16_t>(command.substring(4).toInt());
    mode = ManualMode::SingleLed;
  } else if (command.startsWith("glyph ") && command.length() >= 7) {
    manualGlyph = command.charAt(6);
    mode = ManualMode::Glyph;
  } else if (command == "smile") {
    mode = ManualMode::Smile;
  } else if (command == "off") {
    mode = ManualMode::Off;
    display.clear(true);
  } else if (command == "sync") {
    if (WiFi.status() == WL_CONNECTED) {
      clockReady = syncClock();
      schedule.refreshFromNetwork();
    }
  } else if (command == "status") {
    printStatus();
  } else if (command == "help") {
    printHelp();
  } else {
    Serial.println("Unknown command; type 'help'");
  }
}

void handleSerial() {
  static String buffer;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (buffer.length() > 0) {
        handleCommand(buffer);
        buffer = "";
      }
    } else {
      buffer += c;
    }
  }
}

void renderManual(uint32_t nowMs) {
  switch (mode) {
    case ManualMode::Auto:
      renderAuto(nowMs);
      break;
    case ManualMode::LedScan: {
      const uint16_t index = (nowMs / 1000) % display.ledCount();
      if (index != lastScannedLed) {
        lastScannedLed = index;
        Serial.printf("[scan] LED %u\n", index);
      }
      display.showSingleLed(index, CRGB::White);
      break;
    }
    case ManualMode::SingleLed:
      display.showSingleLed(manualLed, CRGB::White);
      break;
    case ManualMode::Glyph: {
      const auto palette = paletteForRotation(manualGlyph);
      display.renderGlyph(glyphForChar(manualGlyph), palette.dark, palette.light,
                          230, static_cast<uint8_t>(nowMs / 30));
      break;
    }
    case ManualMode::Smile:
      renderSmile(display, nowMs);
      break;
    case ManualMode::Off:
      break;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\nDragon Light booting...");

  // Seed the non-security animation PRNG with device/runtime entropy.
  randomSeed(ESP.getChipId() ^ micros());

  display.begin();
  schedule.begin();
  connectWifi();

  if (WiFi.status() == WL_CONNECTED) {
    clockReady = syncClock();
    lastScheduleAttemptMs = millis();
    const bool refreshed = schedule.refreshFromNetwork();
    tm now{};
    if (refreshed && getDenverTime(now)) lastRefreshDate = isoDate(now);
  }

  setupOta();
  scheduleNextFun(millis());
  printHelp();
}

void loop() {
  serviceNetwork();
  if (otaReady) ArduinoOTA.handle();
  handleSerial();
  renderManual(millis());
  delay(20);
  yield();
}
