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

// Confirmed hardware: NodeMCU v1.0 / ESP8266 driving a 5V WS2812B ECO strip.
// D1 is GPIO5 on the NodeMCU. This is our recommended data pin; verify the
// physical wire is actually connected here before installation.
constexpr uint8_t kLedDataPin = D1;
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

// Brief highlight sweep every 10 minutes. The day letter remains visible.
constexpr uint32_t kFunAnimationDurationMs = 2400;
constexpr uint32_t kFunAnimationMinIntervalMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kFunAnimationMaxIntervalMs = 10UL * 60UL * 1000UL;

struct TransitionWindow {
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
};

// Final five minutes before each class/advisory starts. During these windows the
// day letter temporarily becomes a 5→1 countdown with increasing pulse urgency.
// Break/lunch themselves stay in normal day-letter mode until their final five
// minutes before the next class.
constexpr std::array<TransitionWindow, 6> kTransitions = {{
    {8, 25, 8, 30},    // 1st
    {9, 30, 9, 35},    // 2nd
    {10, 50, 10, 55},  // 3rd, after morning break
    {11, 55, 12, 0},   // Advisory
    {13, 20, 13, 25},  // 4th, after lunch/break
    {14, 25, 14, 30},  // 5th
}};

}  // namespace config
