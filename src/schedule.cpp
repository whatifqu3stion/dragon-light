#include "schedule.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiClientSecureBearSSL.h>

#include "config.h"

bool ScheduleManager::begin() {
  if (!LittleFS.begin()) {
    Serial.println("[schedule] LittleFS mount failed");
    return false;
  }
  return loadCached();
}

bool ScheduleManager::loadCached() {
  if (!LittleFS.exists(config::kScheduleCachePath)) {
    Serial.println("[schedule] No cached schedule yet");
    return false;
  }

  File file = LittleFS.open(config::kScheduleCachePath, "r");
  if (!file) return false;

  csv_ = file.readString();
  file.close();
  Serial.printf("[schedule] Loaded %u cached bytes\n", csv_.length());
  return csv_.length() > 0;
}

bool ScheduleManager::saveCached() const {
  File file = LittleFS.open(config::kScheduleCachePath, "w");
  if (!file) return false;
  const size_t written = file.print(csv_);
  file.close();
  return written == csv_.length();
}

bool ScheduleManager::refreshFromNetwork() {
  if (strlen(config::kScheduleUrl) == 0) {
    Serial.println("[schedule] No remote schedule URL configured");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[schedule] Refresh skipped: Wi-Fi unavailable");
    return false;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  // The feed is public/non-sensitive. This keeps setup simple, but does not
  // authenticate the TLS peer; see the README security note.
  client->setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, config::kScheduleUrl)) {
    Serial.println("[schedule] HTTP begin failed");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[schedule] HTTP GET failed: %d\n", code);
    http.end();
    return false;
  }

  String fresh = http.getString();
  http.end();
  fresh.trim();

  if (!fresh.startsWith("date,")) {
    Serial.println("[schedule] Response did not look like expected CSV");
    return false;
  }

  csv_ = fresh;
  if (!saveCached()) {
    Serial.println("[schedule] Warning: fresh schedule could not be cached");
  }
  Serial.printf("[schedule] Refreshed %u bytes\n", csv_.length());
  return true;
}

String ScheduleManager::cleanField(String value) {
  value.trim();
  if (value.length() >= 2 && value[0] == '"' &&
      value[value.length() - 1] == '"') {
    value = value.substring(1, value.length() - 1);
  }
  value.trim();
  return value;
}

char ScheduleManager::rotationForDate(const String& isoDate) const {
  if (csv_.length() == 0) return '\0';

  int lineStart = 0;
  while (lineStart < csv_.length()) {
    int lineEnd = csv_.indexOf('\n', lineStart);
    if (lineEnd < 0) lineEnd = csv_.length();

    String line = csv_.substring(lineStart, lineEnd);
    line.trim();
    lineStart = lineEnd + 1;

    if (line.length() == 0 || line.startsWith("date,")) continue;

    const int comma1 = line.indexOf(',');
    if (comma1 < 0) continue;
    const int comma2 = line.indexOf(',', comma1 + 1);
    if (comma2 < 0) continue;
    const int comma3 = line.indexOf(',', comma2 + 1);

    const String dateField = cleanField(line.substring(0, comma1));
    if (dateField != isoDate) continue;

    String rotation = cleanField(
        comma3 >= 0 ? line.substring(comma2 + 1, comma3)
                    : line.substring(comma2 + 1));
    rotation.toUpperCase();

    if (rotation == "NONE" || rotation == "NO CLASS") return '-';
    if (rotation.length() >= 1) {
      const char c = rotation[0];
      if (String("BEDRAGON").indexOf(c) >= 0) return c;
    }
    return '\0';
  }

  return '\0';
}
