#include "display.h"
#include "config.h"

namespace {

// Physical LED routing, named from the FRONT of the display. The strip was
// mapped from the back, so left/right have already been flipped here.
// LEDs 4,5,15,16,22 are hidden travel pixels and never intentionally light.
constexpr uint8_t kLowerRightV[] = {0, 1};
constexpr uint8_t kUpperRightV[] = {2, 3};
constexpr uint8_t kTopRightH[] = {6};
constexpr uint8_t kUpperRightDiag[] = {7};
constexpr uint8_t kMiddleRightH[] = {8};
constexpr uint8_t kLowerRightDiag[] = {9};
constexpr uint8_t kBottomRightH[] = {10};
constexpr uint8_t kLowerCenterV[] = {11, 12};
constexpr uint8_t kUpperCenterV[] = {13, 14};
constexpr uint8_t kTopLeftH[] = {17};
constexpr uint8_t kUpperLeftDiag[] = {18};
constexpr uint8_t kMiddleLeftH[] = {19};
constexpr uint8_t kLowerLeftDiag[] = {20};
constexpr uint8_t kBottomLeftH[] = {21};
constexpr uint8_t kLowerLeftV[] = {23, 24};
constexpr uint8_t kUpperLeftV[] = {25, 26};

struct SegmentPixels {
  const uint8_t* pixels;
  uint8_t count;
};

constexpr SegmentPixels kMap[] = {
    {kTopLeftH, 1},       {kTopRightH, 1},
    {kMiddleLeftH, 1},    {kMiddleRightH, 1},
    {kBottomLeftH, 1},    {kBottomRightH, 1},
    {kUpperLeftV, 2},     {kUpperRightV, 2},
    {kLowerLeftV, 2},     {kLowerRightV, 2},
    {kUpperCenterV, 2},   {kLowerCenterV, 2},
    {kUpperLeftDiag, 1},  {kUpperRightDiag, 1},
    {kLowerLeftDiag, 1},  {kLowerRightDiag, 1},
};

static_assert(sizeof(kMap) / sizeof(kMap[0]) ==
                  static_cast<size_t>(Segment::Count),
              "Segment map must cover all 16 logical segments");

constexpr GlyphMask S(Segment s) { return segmentBit(s); }

constexpr GlyphMask kAllHorizontals =
    S(Segment::TopLeftH) | S(Segment::TopRightH) |
    S(Segment::MiddleLeftH) | S(Segment::MiddleRightH) |
    S(Segment::BottomLeftH) | S(Segment::BottomRightH);

constexpr GlyphMask kOuterVerticals =
    S(Segment::UpperLeftV) | S(Segment::UpperRightV) |
    S(Segment::LowerLeftV) | S(Segment::LowerRightV);

}  // namespace

void SegmentDisplay::begin() {
  // Confirmed strip type: 5V WS2812B ECO, normal GRB channel order.
  FastLED.addLeds<WS2812B, config::kLedDataPin, GRB>(leds_, config::kLedCount);
  FastLED.setBrightness(config::kMaxBrightness);
  clear(true);
}

void SegmentDisplay::clear(bool showNow) {
  fill_solid(leds_, config::kLedCount, CRGB::Black);
  if (showNow) show();
}

void SegmentDisplay::show() { FastLED.show(); }

void SegmentDisplay::renderGlyph(GlyphMask glyph, const CRGB& a, const CRGB& b,
                                 uint8_t intensity,
                                 uint8_t gradientPhase) {
  fill_solid(leds_, config::kLedCount, CRGB::Black);

  for (uint8_t i = 0; i < static_cast<uint8_t>(Segment::Count); ++i) {
    if ((glyph & (1u << i)) == 0) continue;

    const auto& mapping = kMap[i];
    for (uint8_t p = 0; p < mapping.count; ++p) {
      const uint8_t blendAmount =
          sin8(static_cast<uint8_t>(gradientPhase + i * 17 + p * 24));
      CRGB color = blend(a, b, blendAmount);
      color.nscale8_video(intensity);
      leds_[mapping.pixels[p]] = color;
    }
  }

  show();
}

void SegmentDisplay::setSegment(Segment segment, const CRGB& color) {
  const auto& mapping = kMap[static_cast<uint8_t>(segment)];
  for (uint8_t p = 0; p < mapping.count; ++p) {
    leds_[mapping.pixels[p]] = color;
  }
}

void SegmentDisplay::showSingleLed(uint16_t index, const CRGB& color) {
  fill_solid(leds_, config::kLedCount, CRGB::Black);
  if (index < config::kLedCount) leds_[index] = color;
  show();
}

uint16_t SegmentDisplay::ledCount() const { return config::kLedCount; }

GlyphMask glyphForChar(char c) {
  c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

  const GlyphMask top = S(Segment::TopLeftH) | S(Segment::TopRightH);
  const GlyphMask middle = S(Segment::MiddleLeftH) | S(Segment::MiddleRightH);
  const GlyphMask bottom = S(Segment::BottomLeftH) | S(Segment::BottomRightH);
  const GlyphMask left = S(Segment::UpperLeftV) | S(Segment::LowerLeftV);
  const GlyphMask right = S(Segment::UpperRightV) | S(Segment::LowerRightV);

  switch (c) {
    case 'B': return kAllHorizontals | kOuterVerticals;
    case 'E': return top | middle | bottom | left;
    case 'D': return top | bottom | left | right;
    case 'R':
      return top | middle | S(Segment::UpperLeftV) |
             S(Segment::UpperRightV) | S(Segment::LowerLeftV) |
             S(Segment::LowerRightDiag);
    case 'A': return top | middle | kOuterVerticals;
    case 'G':
      return top | bottom | left | S(Segment::MiddleRightH) |
             S(Segment::LowerRightV);
    case 'O': return top | bottom | kOuterVerticals;
    case 'N':
      return left | right | S(Segment::UpperLeftDiag) |
             S(Segment::LowerRightDiag);
    case '0': return top | bottom | kOuterVerticals;
    case '1': return right;
    case '2':
      return top | S(Segment::UpperRightV) | middle |
             S(Segment::LowerLeftV) | bottom;
    case '3': return top | right | middle | bottom;
    case '4': return S(Segment::UpperLeftV) | right | middle;
    case '5':
      return top | S(Segment::UpperLeftV) | middle |
             S(Segment::LowerRightV) | bottom;
    case '6': return top | left | middle | S(Segment::LowerRightV) | bottom;
    case '7': return top | right;
    case '8': return kAllHorizontals | kOuterVerticals;
    case '9': return top | S(Segment::UpperLeftV) | right | middle | bottom;
    case '-': return middle;
    default: return 0;
  }
}

GlyphMask smileGlyph() {
  // Slanted eyes plus a U-shaped mouth, constrained by the segment geometry.
  return S(Segment::UpperLeftDiag) | S(Segment::UpperRightDiag) |
         S(Segment::LowerLeftV) | S(Segment::LowerRightV) |
         S(Segment::BottomLeftH) | S(Segment::BottomRightH);
}

GlyphMask dashGlyph() { return glyphForChar('-'); }
