#pragma once

#include <Arduino.h>
#include <FastLED.h>

enum class Segment : uint8_t {
  TopLeftH = 0,
  TopRightH,
  MiddleLeftH,
  MiddleRightH,
  BottomLeftH,
  BottomRightH,
  UpperLeftV,
  UpperRightV,
  LowerLeftV,
  LowerRightV,
  UpperCenterV,
  LowerCenterV,
  UpperLeftDiag,
  UpperRightDiag,
  LowerLeftDiag,
  LowerRightDiag,
  Count
};

using GlyphMask = uint16_t;

constexpr GlyphMask segmentBit(Segment s) {
  return static_cast<GlyphMask>(1u << static_cast<uint8_t>(s));
}

class SegmentDisplay {
 public:
  void begin();
  void clear(bool showNow = true);
  void show();
  void renderGlyph(GlyphMask glyph, const CRGB& a, const CRGB& b,
                   uint8_t intensity, uint8_t gradientPhase);
  void setSegment(Segment segment, const CRGB& color);
  void showSingleLed(uint16_t index, const CRGB& color);
  uint16_t ledCount() const;

 private:
  CRGB leds_[27]{};
};

GlyphMask glyphForChar(char c);
GlyphMask smileGlyph();
GlyphMask dashGlyph();
