#pragma once

#include <Arduino.h>

class ScheduleManager {
 public:
  bool begin();
  bool loadCached();
  bool refreshFromNetwork(const String& isoDate);

  // Returns B/E/D/R/A/G/O/N, '-' for NONE/no-class, or '\0' if missing.
  char rotationForDate(const String& isoDate) const;
  const char* sourceForDate(const String& isoDate) const;

  bool hasSchedule() const {
    return csv_.length() > 0 || calendarRotation_ != '\0';
  }

 private:
  bool saveCached() const;
  bool refreshCsv();
  bool refreshCalendar(const String& isoDate);
  char csvRotationForDate(const String& isoDate) const;
  static String cleanField(String value);
  String csv_;
  String calendarDate_;
  char calendarRotation_ = '\0';
  mutable String lastMismatchDate_;
};
