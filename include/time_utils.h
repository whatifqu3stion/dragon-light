#pragma once

#include <Arduino.h>
#include <time.h>
#include "config.h"

bool syncClock();
bool getDenverTime(tm& out);
String isoDate(const tm& t);
int minutesSinceMidnight(const tm& t);
bool isWeekend(const tm& t);
bool isAtOrAfter(int nowMinutes, uint8_t hour, uint8_t minute);
bool isBefore(int nowMinutes, uint8_t hour, uint8_t minute);
