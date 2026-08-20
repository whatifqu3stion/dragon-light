#pragma once

#include <Arduino.h>

class ScheduleManager {
 public:
  bool begin();
  bool loadCached();
  bool refreshFromNetwork();

  // Returns B/E/D/R/A/G/O/N, '-' for NONE/no-class, or '\0' if missing.
  char rotationForDate(const String& isoDate) const;

  bool hasSchedule() const { return csv_.length() > 0; }

 private:
  bool saveCached() const;
  static String cleanField(String value);
  String csv_;
};
