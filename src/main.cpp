#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <cctype>
#include <cstring>

#include "config.h"
#include "display.h"
#include "effects.h"
#include "schedule.h"
#include "time_utils.h"
#include "web_ui.h"

namespace {

SegmentDisplay display;
ScheduleManager schedule;
ESP8266WebServer webServer(80);

enum class ManualMode {
  Auto,
  LedScan,
  SingleLed,
  Glyph,
  Spinner,
  Celebration,
  Off
};
ManualMode mode = ManualMode::Auto;
uint16_t manualLed = 0;
char manualGlyph = 'B';
uint16_t lastScannedLed = 0xFFFF;

bool clockReady = false;
bool otaReady = false;
bool webReady = false;
String lastRefreshDate;
uint32_t lastScheduleAttemptMs = 0;
uint32_t lastClockAttemptMs = 0;
uint32_t funStartedAt = 0;
uint32_t funEndsAt = 0;
uint32_t nextFunAt = 0;

const char* modeName() {
  switch (mode) {
    case ManualMode::Auto: return "auto";
    case ManualMode::LedScan: return "scan";
    case ManualMode::SingleLed: return "single LED";
    case ManualMode::Glyph: return "glyph";
    case ManualMode::Spinner: return "spin preview";
    case ManualMode::Celebration: return "celebration preview";
    case ManualMode::Off: return "off";
  }
  return "unknown";
}

bool isRotationLetter(char value) {
  value = static_cast<char>(toupper(static_cast<unsigned char>(value)));
  return strchr("BEDRAGON", value) != nullptr;
}

bool isSupportedGlyph(char value) {
  value = static_cast<char>(toupper(static_cast<unsigned char>(value)));
  return strchr("BEDRAGON0123456789-", value) != nullptr;
}

bool parseUnsignedInRange(String text, int minimum, int maximum, int& result) {
  text.trim();
  if (text.length() == 0) return false;
  for (size_t i = 0; i < text.length(); ++i) {
    if (!isDigit(text.charAt(i))) return false;
  }
  const long value = text.toInt();
  if (value < minimum || value > maximum) return false;
  result = static_cast<int>(value);
  return true;
}

char currentRotation() {
  tm now{};
  if (!getDenverTime(now)) return '\0';
  return schedule.rotationForDate(isoDate(now));
}

void selectSpinner() {
  const char rotation = currentRotation();
  if (isRotationLetter(rotation)) manualGlyph = rotation;
  mode = ManualMode::Spinner;
}

bool saveBrightness() {
  File file = LittleFS.open(config::kBrightnessPath, "w");
  if (!file) return false;
  const size_t written =
      file.printf("%u\n", static_cast<unsigned int>(display.brightness()));
  file.close();
  return written > 0;
}

void loadBrightness() {
  File file = LittleFS.open(config::kBrightnessPath, "r");
  if (!file) {
    Serial.printf("[settings] Using default brightness %u\n",
                  config::kDefaultBrightness);
    return;
  }

  const int value = file.parseInt();
  file.close();
  if (value < config::kMinBrightness || value > config::kMaxBrightness) {
    Serial.println("[settings] Ignoring invalid saved brightness");
    return;
  }
  display.setBrightness(static_cast<uint8_t>(value));
  Serial.printf("[settings] Loaded brightness %d\n", value);
}

uint32_t randomFunDelay() {
  const uint32_t span = config::kFunAnimationMaxIntervalMs -
                        config::kFunAnimationMinIntervalMs;
  return config::kFunAnimationMinIntervalMs + random(span + 1);
}

void scheduleNextFun(uint32_t nowMs) { nextFunAt = nowMs + randomFunDelay(); }

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(config::kHostname);
  WiFi.setAutoReconnect(true);

  WiFiManager manager;
  manager.setConnectTimeout(config::kWifiConnectTimeoutSeconds);
  manager.setConfigPortalTimeout(config::kConfigPortalTimeoutSeconds);

  Serial.println("[wifi] Trying saved Wi-Fi credentials");
  Serial.printf("[wifi] If needed, setup AP: %s\n", config::kSetupApName);

  bool connected = false;
  const size_t setupPasswordLength = strlen(config::kSetupApPassword);
  if (setupPasswordLength >= 8) {
    connected = manager.autoConnect(config::kSetupApName,
                                    config::kSetupApPassword);
  } else {
    if (setupPasswordLength > 0) {
      Serial.println("[wifi] Setup password is under 8 characters; using open setup AP");
    }
    connected = manager.autoConnect(config::kSetupApName);
  }

  if (connected && WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] Connected to %s: %s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[wifi] Setup/connect timed out; cached schedule remains usable");
  }
}

void resetWifi() {
  WiFiManager manager;
  manager.resetSettings();
  Serial.println("[wifi] Saved credentials cleared; restarting into setup mode");
  delay(500);
  ESP.restart();
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

bool authorizeWebRequest() {
  if (strlen(config::kOtaPassword) == 0 ||
      webServer.authenticate("dragon", config::kOtaPassword)) {
    return true;
  }
  webServer.requestAuthentication();
  return false;
}

String statusJson() {
  tm now{};
  const bool hasTime = getDenverTime(now);
  const String today = hasTime ? isoDate(now) : String();
  const char rotation = hasTime ? schedule.rotationForDate(today) : '\0';
  char timeText[24] = "unavailable";
  if (hasTime) {
    snprintf(timeText, sizeof(timeText), "%s %02d:%02d:%02d",
             today.c_str(), now.tm_hour, now.tm_min, now.tm_sec);
  }

  String json;
  json.reserve(220);
  json += F("{\"brightness\":");
  json += static_cast<unsigned int>(display.brightness());
  json += F(",\"mode\":\"");
  json += modeName();
  json += F("\",\"rotation\":\"");
  json += rotation ? rotation : '?';
  json += F("\",\"source\":\"");
  json += hasTime ? schedule.sourceForDate(today) : "none";
  json += F("\",\"time\":\"");
  json += timeText;
  json += F("\",\"ip\":\"");
  json += WiFi.status() == WL_CONNECTED
              ? WiFi.localIP().toString()
              : String("disconnected");
  json += F("\"}");
  return json;
}

void sendStatusJson() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", statusJson());
}

void setupWebControl() {
  if (webReady || WiFi.status() != WL_CONNECTED) return;

  webServer.on("/", HTTP_GET, []() {
    if (!authorizeWebRequest()) return;
    webServer.send_P(200, PSTR("text/html; charset=utf-8"), kControlPage);
  });
  webServer.on("/api/status", HTTP_GET, []() {
    if (!authorizeWebRequest()) return;
    sendStatusJson();
  });
  webServer.on("/api/brightness", HTTP_POST, []() {
    if (!authorizeWebRequest()) return;
    int value = 0;
    if (!parseUnsignedInRange(webServer.arg("value"),
                              config::kMinBrightness,
                              config::kMaxBrightness, value)) {
      webServer.send(400, "text/plain", "Brightness must be 5 through 128.");
      return;
    }
    display.setBrightness(static_cast<uint8_t>(value));
    if (!saveBrightness()) {
      Serial.println("[settings] Warning: brightness could not be saved");
    }
    Serial.printf("[web] Brightness %d\n", value);
    sendStatusJson();
  });
  webServer.on("/api/mode", HTTP_POST, []() {
    if (!authorizeWebRequest()) return;
    String requested = webServer.arg("value");
    requested.toLowerCase();
    if (requested == "auto") {
      mode = ManualMode::Auto;
    } else if (requested == "off") {
      mode = ManualMode::Off;
      display.clear(true);
    } else if (requested == "spin") {
      selectSpinner();
    } else if (requested == "celebrate") {
      mode = ManualMode::Celebration;
    } else {
      webServer.send(400, "text/plain", "Unknown display mode.");
      return;
    }
    Serial.printf("[web] Mode %s\n", modeName());
    sendStatusJson();
  });
  webServer.on("/favicon.ico", HTTP_GET, []() {
    if (!authorizeWebRequest()) return;
    webServer.send(204);
  });
  webServer.onNotFound([]() {
    if (!authorizeWebRequest()) return;
    webServer.send(404, "text/plain", "Not found");
  });
  webServer.begin();

  if (!otaReady) MDNS.begin(config::kHostname);
  MDNS.addService("http", "tcp", 80);
  webReady = true;
  Serial.printf("[web] Ready at http://%s.local\n", config::kHostname);
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
  if (schedule.refreshFromNetwork(today)) lastRefreshDate = today;
}

void serviceNetwork() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!clockReady &&
      (lastClockAttemptMs == 0 || millis() - lastClockAttemptMs > 60000UL)) {
    lastClockAttemptMs = millis();
    clockReady = syncClock();
  }

  if (!otaReady && strlen(config::kOtaPassword) > 0) setupOta();
  if (!webReady) setupWebControl();
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
    renderCelebration(display, nowMs);
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
    Serial.printf("[status] Clock unavailable | brightness=%u mode=%s | wifi=%s | ota=%s | web=%s\n",
                  display.brightness(), modeName(),
                  WiFi.status() == WL_CONNECTED ? "up" : "down",
                  otaReady ? "ready" : "off",
                  webReady ? "ready" : "off");
    return;
  }
  const String today = isoDate(now);
  const char rotation = schedule.rotationForDate(today);
  Serial.printf("[status] %s %02d:%02d:%02d | rotation=%c source=%s | brightness=%u mode=%s | wifi=%s | ota=%s | web=%s\n",
                today.c_str(), now.tm_hour, now.tm_min, now.tm_sec,
                rotation ? rotation : '?',
                schedule.sourceForDate(today),
                display.brightness(), modeName(),
                WiFi.status() == WL_CONNECTED ? "up" : "down",
                otaReady ? "ready" : "off",
                webReady ? "ready" : "off");
}

void printHelp() {
  Serial.println("Commands: auto | off | brightness N | spin [X] | celebrate");
  Serial.println("          scan | led N | glyph X | sync | resetwifi | status | help");
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) return;

  String normalized = command;
  normalized.toLowerCase();

  if (normalized == "auto") {
    mode = ManualMode::Auto;
    Serial.println("[mode] Auto");
  } else if (normalized == "scan") {
    mode = ManualMode::LedScan;
    lastScannedLed = 0xFFFF;
  } else if (normalized.startsWith("led ")) {
    int value = 0;
    if (!parseUnsignedInRange(command.substring(4), 0,
                              display.ledCount() - 1, value)) {
      Serial.printf("[command] LED must be 0 through %u\n",
                    display.ledCount() - 1);
      return;
    }
    manualLed = static_cast<uint16_t>(value);
    mode = ManualMode::SingleLed;
  } else if (normalized.startsWith("glyph ") && command.length() >= 7) {
    const char glyph = static_cast<char>(
        toupper(static_cast<unsigned char>(command.charAt(6))));
    if (!isSupportedGlyph(glyph)) {
      Serial.println("[command] Glyph must be B/E/D/R/A/G/O/N, 0-9, or -");
      return;
    }
    manualGlyph = glyph;
    mode = ManualMode::Glyph;
  } else if (normalized == "spin" || normalized.startsWith("spin ")) {
    if (command.length() >= 6) {
      const char rotation = static_cast<char>(
          toupper(static_cast<unsigned char>(command.charAt(5))));
      if (!isRotationLetter(rotation)) {
        Serial.println("[command] Spin color must be B/E/D/R/A/G/O/N");
        return;
      }
      manualGlyph = rotation;
      mode = ManualMode::Spinner;
    } else {
      selectSpinner();
    }
  } else if (normalized == "celebrate") {
    mode = ManualMode::Celebration;
  } else if (normalized == "off") {
    mode = ManualMode::Off;
    display.clear(true);
  } else if (normalized.startsWith("brightness ")) {
    int value = 0;
    if (!parseUnsignedInRange(command.substring(11),
                              config::kMinBrightness,
                              config::kMaxBrightness, value)) {
      Serial.printf("[command] Brightness must be %u through %u\n",
                    config::kMinBrightness, config::kMaxBrightness);
      return;
    }
    display.setBrightness(static_cast<uint8_t>(value));
    if (!saveBrightness()) {
      Serial.println("[settings] Warning: brightness could not be saved");
    }
    Serial.printf("[settings] Brightness %d saved\n", value);
  } else if (normalized == "sync") {
    if (WiFi.status() == WL_CONNECTED) {
      clockReady = syncClock();
      tm now{};
      const String today = getDenverTime(now) ? isoDate(now) : String();
      schedule.refreshFromNetwork(today);
    } else {
      Serial.println("[sync] Wi-Fi is disconnected");
    }
  } else if (normalized == "resetwifi") {
    resetWifi();
  } else if (normalized == "status") {
    printStatus();
  } else if (normalized == "help") {
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
    case ManualMode::Spinner:
      renderFinalMinuteSpinner(display, manualGlyph, nowMs);
      break;
    case ManualMode::Celebration:
      renderCelebration(display, nowMs);
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
  loadBrightness();
  connectWifi();

  if (WiFi.status() == WL_CONNECTED) {
    clockReady = syncClock();
    lastScheduleAttemptMs = millis();
    tm now{};
    const String today = getDenverTime(now) ? isoDate(now) : String();
    const bool refreshed = schedule.refreshFromNetwork(today);
    if (refreshed && today.length() > 0) lastRefreshDate = today;
  }

  setupOta();
  setupWebControl();
  scheduleNextFun(millis());
  printHelp();
}

void loop() {
  serviceNetwork();
  if (otaReady) ArduinoOTA.handle();
  if (webReady) {
    webServer.handleClient();
    MDNS.update();
  }
  handleSerial();
  renderManual(millis());
  delay(20);
  yield();
}
