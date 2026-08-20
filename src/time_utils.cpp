#include "time_utils.h"

bool syncClock() {
  configTzTime(config::kTimezone, config::kNtpPrimary, config::kNtpSecondary);

  const uint32_t started = millis();
  tm current{};
  while (millis() - started < config::kTimeSyncTimeoutMs) {
    if (getLocalTime(&current, 250)) {
      Serial.printf("[time] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                    current.tm_year + 1900, current.tm_mon + 1,
                    current.tm_mday, current.tm_hour, current.tm_min,
                    current.tm_sec);
      return true;
    }
    delay(100);
  }

  Serial.println("[time] NTP sync timed out");
  return false;
}

bool getDenverTime(tm& out) { return getLocalTime(&out, 20); }

String isoDate(const tm& t) {
  char buffer[11];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &t);
  return String(buffer);
}

int minutesSinceMidnight(const tm& t) { return t.tm_hour * 60 + t.tm_min; }

bool isWeekend(const tm& t) { return t.tm_wday == 0 || t.tm_wday == 6; }

bool isAtOrAfter(int nowMinutes, uint8_t hour, uint8_t minute) {
  return nowMinutes >= hour * 60 + minute;
}

bool isBefore(int nowMinutes, uint8_t hour, uint8_t minute) {
  return nowMinutes < hour * 60 + minute;
}
