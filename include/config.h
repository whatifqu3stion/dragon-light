#pragma once

#include <Arduino.h>
#include <array>

#if __has_include("local_config.h")
#include "local_config.h"
#endif

#ifndef DRAGON_LIGHT_WIFI_SSID
#define DRAGON_LIGHT_WIFI_SSID ""
#endif
#ifndef DRAGON_LIGHT_WIFI_PASSWORD
#define DRAGON_LIGHT_WIFI_PASSWORD ""
#endif
#ifndef DRAGON_LIGHT_OTA_PASSWORD
#define DRAGON_LIGHT_OTA_PASSWORD ""
#endif
#ifndef DRAGON_LIGHT_SCHEDULE_URL
#define DRAGON_LIGHT_SCHEDULE_URL ""
#endif

namespace config {

constexpr char kWifiSsid[] = DRAGON_LIGHT_WIFI_SSID;
constexpr char kWifiPassword[] = DRAGON_LIGHT_WIFI_PASSWORD;
constexpr char kOtaPassword[] = DRAGON_LIGHT_OTA_PASSWORD;
constexpr char kScheduleUrl[] = DRAGON_LIGHT_SCHEDULE_URL;

// Hardware assumptions: verify these on the physical unit before installation.
constexpr uint8_t kLedDataPin = 5;
constexpr uint16_t kLedCount = 26;
constexpr uint8_t kMaxBrightness = 128;
constexpr char kHostname[] = "dragon-light";
constexpr char kScheduleCachePath[] = "/schedule.csv";

// Denver/Mountain Time, including US daylight-saving transitions.
constexpr char kTimezone[] = "MST7MDT,M3.2.0/2,M11.1.0/2";
constexpr char kNtpPrimary[] = "pool.ntp.org";
constexpr char kNtpSecondary[] = "time.google.com";

// Daily behavior.
constexpr uint8_t kScheduleRefreshHour = 7;
constexpr uint8_t kScheduleRefreshMinute = 15;
constexpr uint8_t kWakeHour = 7;
constexpr uint8_t kWakeMinute = 30;
constexpr uint8_t kCelebrationHour = 15;
constexpr uint8_t kCelebrationMinute = 30;
constexpr uint8_t kSleepHour = 15;
constexpr uint8_t kSleepMinute = 45;

constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kTimeSyncTimeoutMs = 12000;

// Small ambient flourish: brief and intentionally infrequent.
constexpr uint32_t kFunAnimationDurationMs = 2400;
constexpr uint32_t kFunAnimationMinIntervalMs = 12UL * 60UL * 1000UL;
constexpr uint32_t kFunAnimationMaxIntervalMs = 20UL * 60UL * 1000UL;

// Populate these once the real class transition windows are known.
struct TransitionWindow {
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
};

constexpr std::array<TransitionWindow, 0> kTransitions = {};

}  // namespace config
