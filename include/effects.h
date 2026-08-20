#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "display.h"

struct DayPalette {
  CRGB dark;
  CRGB light;
  CRGB accent;
};

DayPalette paletteForRotation(char rotation);
void renderDayLetter(SegmentDisplay& display, char rotation, uint32_t nowMs);
void renderCountdown(SegmentDisplay& display, char rotation, uint8_t minutesLeft,
                     uint32_t secondsLeft, uint32_t nowMs);
void renderSmile(SegmentDisplay& display, uint32_t nowMs);
void renderFunSweep(SegmentDisplay& display, char rotation, uint32_t nowMs,
                    uint32_t animationStartMs);
void renderUnknown(SegmentDisplay& display, uint32_t nowMs);
