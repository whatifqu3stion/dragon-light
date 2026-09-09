#include "schedule.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiClientSecureBearSSL.h>
#include <cstring>

#include "config.h"

namespace {

constexpr uint16_t kGoogleHttpTimeoutMs = 45000;
constexpr uint32_t kCalendarStallTimeoutMs = 45000UL;

bool isRotationLetter(char value) {
  return strchr("BEDRAGON", value) != nullptr;
}

String fieldAt(const String& line, int fieldIndex) {
  int start = 0;
  int current = 0;
  bool quoted = false;

  const int lineLength = static_cast<int>(line.length());
  for (int i = 0; i <= lineLength; ++i) {
    const char c = i < lineLength ? line[i] : ',';
    if (c == '"') quoted = !quoted;
    if (c == ',' && !quoted) {
      if (current == fieldIndex) return line.substring(start, i);
      start = i + 1;
      ++current;
    }
  }
  return String();
}

int findHeaderField(const String& header, const char* wanted) {
  for (int index = 0; index < 12; ++index) {
    String field = fieldAt(header, index);
    field.trim();
    field.toLowerCase();
    if (field == wanted) return index;
    if (field.length() == 0 && index > 0) break;
  }
  return -1;
}

bool looksLikeScheduleCsv(const String& content) {
  int end = content.indexOf('\n');
  if (end < 0) end = content.length();
  String header = content.substring(0, end);
  header.replace("\r", "");
  if (header.length() >= 3 &&
      static_cast<uint8_t>(header[0]) == 0xEF &&
      static_cast<uint8_t>(header[1]) == 0xBB &&
      static_cast<uint8_t>(header[2]) == 0xBF) {
    header.remove(0, 3);
  }
  return findHeaderField(header, "date") >= 0 &&
         findHeaderField(header, "rotation") >= 0;
}

String compactDate(const String& isoDate) {
  String result = isoDate;
  result.replace("-", "");
  return result;
}

char rotationFromSummary(const String& summary) {
  if (summary.length() != 5 || !summary.startsWith("Day ")) return '\0';
  const char rotation = summary[4];
  return isRotationLetter(rotation) ? rotation : '\0';
}

void processCalendarLine(const String& line, const String& targetDate,
                         bool& inEvent, String& eventDate,
                         String& eventSummary, char& foundRotation,
                         bool& conflict) {
  if (line == "BEGIN:VEVENT") {
    inEvent = true;
    eventDate = "";
    eventSummary = "";
    return;
  }

  if (line == "END:VEVENT") {
    if (inEvent && eventDate == targetDate) {
      const char candidate = rotationFromSummary(eventSummary);
      if (candidate != '\0') {
        if (foundRotation != '\0' && foundRotation != candidate) {
          conflict = true;
        } else {
          foundRotation = candidate;
        }
      }
    }
    inEvent = false;
    return;
  }

  if (!inEvent) return;
  const int colon = line.indexOf(':');
  if (colon < 0) return;

  String property = line.substring(0, colon);
  property.toUpperCase();
  const String value = line.substring(colon + 1);

  if (property == "SUMMARY" || property.startsWith("SUMMARY;")) {
    eventSummary = value;
  } else if (property == "DTSTART" || property.startsWith("DTSTART;")) {
    // Date-only DTSTART values are all-day events. Timed events contain T.
    if (value.length() == 8 && value.indexOf('T') < 0) eventDate = value;
  }
}

}  // namespace

bool ScheduleManager::begin() {
  if (!LittleFS.begin()) {
    Serial.println("[schedule] LittleFS mount failed");
    return false;
  }
  return loadCached();
}

bool ScheduleManager::loadCached() {
  if (!LittleFS.exists(config::kScheduleCachePath)) {
    Serial.println("[schedule] No cached CSV schedule yet");
    return false;
  }

  File file = LittleFS.open(config::kScheduleCachePath, "r");
  if (!file) return false;

  csv_ = file.readString();
  file.close();
  Serial.printf("[schedule] Loaded %u cached CSV bytes\n", csv_.length());
  return csv_.length() > 0;
}

bool ScheduleManager::saveCached() const {
  File file = LittleFS.open(config::kScheduleCachePath, "w");
  if (!file) return false;
  const size_t written = file.print(csv_);
  file.close();
  return written == csv_.length();
}

bool ScheduleManager::refreshCsv() {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(
      new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient http;
  // Google can take well over the ESP8266HTTPClient default timeout to begin
  // returning these public feeds. HTTP/1.0 also avoids keeping an unknown-size
  // response open after its body has completed.
  http.useHTTP10(true);
  http.setTimeout(kGoogleHttpTimeoutMs);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, config::kScheduleUrl)) {
    Serial.println("[schedule] CSV HTTP begin failed");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[schedule] CSV HTTP GET failed: %d\n", code);
    http.end();
    return false;
  }

  String fresh = http.getString();
  http.end();
  fresh.trim();

  if (!looksLikeScheduleCsv(fresh)) {
    Serial.printf("[schedule] Response did not look like expected CSV (%u bytes)\n",
                  fresh.length());
    return false;
  }

  csv_ = fresh;
  if (!saveCached()) {
    Serial.println("[schedule] Warning: fresh CSV could not be cached");
  }
  Serial.printf("[schedule] Refreshed %u CSV bytes\n", csv_.length());
  return true;
}

bool ScheduleManager::refreshCalendar(const String& isoDate) {
  if (isoDate.length() != 10) {
    Serial.println("[calendar] Refresh skipped: current date unavailable");
    return false;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(
      new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(kGoogleHttpTimeoutMs);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, config::kCalendarUrl)) {
    Serial.println("[calendar] HTTP begin failed");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[calendar] HTTP GET failed: %d\n", code);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  stream->setTimeout(kGoogleHttpTimeoutMs);
  int remaining = http.getSize();
  uint32_t lastDataAt = millis();
  String logicalLine;
  bool logicalLineTruncated = false;
  bool inEvent = false;
  bool sawCalendar = false;
  String eventDate;
  String eventSummary;
  char foundRotation = '\0';
  bool conflict = false;
  const String targetDate = compactDate(isoDate);

  while ((http.connected() || stream->available() > 0) &&
         (remaining > 0 || remaining == -1)) {
    const size_t available = stream->available();
    if (available == 0) {
      if (millis() - lastDataAt > kCalendarStallTimeoutMs) {
        Serial.println("[calendar] Read timed out");
        http.end();
        return false;
      }
      delay(1);
      yield();
      continue;
    }

    String physicalLine = stream->readStringUntil('\n');
    lastDataAt = millis();
    if (remaining > 0) remaining -= physicalLine.length() + 1;
    if (physicalLine.endsWith("\r")) {
      physicalLine.remove(physicalLine.length() - 1);
    }
    if (!sawCalendar && logicalLine.length() == 0 &&
        physicalLine.length() >= 3 &&
        static_cast<uint8_t>(physicalLine[0]) == 0xEF &&
        static_cast<uint8_t>(physicalLine[1]) == 0xBB &&
        static_cast<uint8_t>(physicalLine[2]) == 0xBF) {
      physicalLine.remove(0, 3);
    }

    const bool continuation = physicalLine.startsWith(" ") ||
                              physicalLine.startsWith("\t");
    if (continuation) {
      if (!logicalLineTruncated &&
          logicalLine.length() + physicalLine.length() <= 256) {
        logicalLine += physicalLine.substring(1);
      } else {
        logicalLineTruncated = true;
      }
      continue;
    }

    if (logicalLine.length() > 0 && !logicalLineTruncated) {
      if (logicalLine == "BEGIN:VCALENDAR") sawCalendar = true;
      processCalendarLine(logicalLine, targetDate, inEvent, eventDate,
                          eventSummary, foundRotation, conflict);
    }
    logicalLine = physicalLine;
    logicalLineTruncated = logicalLine.length() > 256;
    if (logicalLineTruncated) logicalLine = "";
  }

  if (logicalLine.length() > 0 && !logicalLineTruncated) {
    if (logicalLine == "BEGIN:VCALENDAR") sawCalendar = true;
    processCalendarLine(logicalLine, targetDate, inEvent, eventDate,
                        eventSummary, foundRotation, conflict);
  }
  http.end();

  if (!sawCalendar) {
    Serial.println("[calendar] Response did not look like an iCal/ICS feed");
    return false;
  }

  if (conflict) {
    Serial.printf("[calendar] Conflicting Day events for %s; using CSV fallback\n",
                  isoDate.c_str());
    foundRotation = '\0';
  }

  calendarDate_ = isoDate;
  calendarRotation_ = foundRotation;
  lastMismatchDate_ = "";
  if (foundRotation == '\0') {
    Serial.printf("[calendar] No exact all-day Day X event for %s\n",
                  isoDate.c_str());
  } else {
    Serial.printf("[calendar] %s is Day %c\n", isoDate.c_str(),
                  foundRotation);
  }
  return true;
}

bool ScheduleManager::refreshFromNetwork(const String& isoDate) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[schedule] Refresh skipped: Wi-Fi unavailable");
    return false;
  }

  const bool csvConfigured = strlen(config::kScheduleUrl) > 0;
  const bool calendarConfigured = strlen(config::kCalendarUrl) > 0;
  if (!csvConfigured && !calendarConfigured) {
    Serial.println("[schedule] No remote schedule sources configured");
    return false;
  }

  bool allConfiguredSourcesSucceeded = true;
  if (csvConfigured && !refreshCsv()) allConfiguredSourcesSucceeded = false;
  if (calendarConfigured && !refreshCalendar(isoDate)) {
    allConfiguredSourcesSucceeded = false;
  }
  return allConfiguredSourcesSucceeded;
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

char ScheduleManager::csvRotationForDate(const String& isoDate) const {
  if (csv_.length() == 0) return '\0';

  int lineStart = 0;
  const int csvLength = static_cast<int>(csv_.length());
  int dateColumn = -1;
  int rotationColumn = -1;
  while (lineStart < csvLength) {
    int lineEnd = csv_.indexOf('\n', lineStart);
    if (lineEnd < 0) lineEnd = csvLength;

    String line = csv_.substring(lineStart, lineEnd);
    line.trim();
    lineStart = lineEnd + 1;
    if (line.length() == 0) continue;

    if (dateColumn < 0 || rotationColumn < 0) {
      if (line.length() >= 3 &&
          static_cast<uint8_t>(line[0]) == 0xEF &&
          static_cast<uint8_t>(line[1]) == 0xBB &&
          static_cast<uint8_t>(line[2]) == 0xBF) {
        line.remove(0, 3);
      }
      dateColumn = findHeaderField(line, "date");
      rotationColumn = findHeaderField(line, "rotation");
      continue;
    }

    if (cleanField(fieldAt(line, dateColumn)) != isoDate) continue;

    String rotation = cleanField(fieldAt(line, rotationColumn));
    rotation.toUpperCase();
    if (rotation == "NONE" || rotation == "NO CLASS") return '-';
    if (rotation.length() >= 1 && isRotationLetter(rotation[0])) {
      return rotation[0];
    }
    return '\0';
  }

  return '\0';
}

char ScheduleManager::rotationForDate(const String& isoDate) const {
  const char csvRotation = csvRotationForDate(isoDate);
  if (calendarDate_ != isoDate || calendarRotation_ == '\0') {
    return csvRotation;
  }

  if (isRotationLetter(csvRotation) && csvRotation != calendarRotation_ &&
      lastMismatchDate_ != isoDate) {
    Serial.printf("[schedule] Mismatch for %s: calendar=%c CSV=%c; calendar wins\n",
                  isoDate.c_str(), calendarRotation_, csvRotation);
    lastMismatchDate_ = isoDate;
  }
  return calendarRotation_;
}

const char* ScheduleManager::sourceForDate(const String& isoDate) const {
  if (calendarDate_ == isoDate && calendarRotation_ != '\0') return "calendar";
  if (csvRotationForDate(isoDate) != '\0') return "csv";
  return "none";
}
