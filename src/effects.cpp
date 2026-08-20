#include "effects.h"

namespace {

uint8_t slowBreath(uint32_t nowMs, uint8_t low = 205, uint8_t high = 255,
                   uint32_t periodMs = 30000) {
  const uint8_t phase = static_cast<uint8_t>((nowMs * 256ULL) / periodMs);
  return low + scale8(sin8(phase), high - low);
}

uint8_t gradientPhase(uint32_t nowMs, uint32_t periodMs = 36000) {
  return static_cast<uint8_t>((nowMs * 256ULL) / periodMs);
}

}  // namespace

DayPalette paletteForRotation(char rotation) {
  switch (toupper(static_cast<unsigned char>(rotation))) {
    case 'B': return {CRGB(12, 45, 170), CRGB(70, 220, 255), CRGB(220, 245, 255)};
    case 'E': return {CRGB(0, 80, 48), CRGB(25, 220, 125), CRGB(185, 255, 220)};
    case 'D': return {CRGB(70, 15, 105), CRGB(170, 75, 230), CRGB(225, 190, 255)};
    case 'R': return {CRGB(110, 0, 25), CRGB(235, 30, 45), CRGB(255, 145, 105)};
    case 'A': return {CRGB(155, 55, 0), CRGB(255, 155, 20), CRGB(255, 235, 120)};
    case 'G': return {CRGB(0, 75, 65), CRGB(50, 190, 80), CRGB(185, 230, 80)};
    case 'O': return {CRGB(175, 45, 0), CRGB(255, 105, 10), CRGB(255, 195, 135)};
    case 'N': return {CRGB(20, 20, 75), CRGB(80, 80, 210), CRGB(175, 170, 255)};
    default: return {CRGB(50, 50, 50), CRGB(130, 130, 130), CRGB::White};
  }
}

void renderDayLetter(SegmentDisplay& display, char rotation, uint32_t nowMs) {
  const auto palette = paletteForRotation(rotation);
  display.renderGlyph(glyphForChar(rotation), palette.dark, palette.light,
                      slowBreath(nowMs), gradientPhase(nowMs));
}

void renderCountdown(SegmentDisplay& display, char rotation, uint8_t minutesLeft,
                     uint32_t secondsLeft, uint32_t nowMs) {
  const auto palette = paletteForRotation(rotation);
  minutesLeft = constrain(minutesLeft, 1, 9);

  // Urgency rises through the final five minutes: faster pulse and more of the
  // pale accent color, while preserving the current rotation day's identity.
  const uint32_t clamped = secondsLeft < 300UL ? secondsLeft : 300UL;
  const uint8_t urgency = static_cast<uint8_t>(255 - (clamped * 255UL) / 300UL);
  const uint32_t periodMs = map(urgency, 0, 255, 4500, 650);
  const uint8_t phase = static_cast<uint8_t>((nowMs * 256ULL) / periodMs);
  const uint8_t pulse = 165 + scale8(sin8(phase), 90);
  const CRGB light = blend(palette.light, palette.accent, urgency);

  display.renderGlyph(glyphForChar('0' + minutesLeft), palette.dark, light,
                      pulse, gradientPhase(nowMs, 5500));
}

void renderSmile(SegmentDisplay& display, uint32_t nowMs) {
  const uint8_t hue = static_cast<uint8_t>((nowMs / 45) & 0xFF);
  display.renderGlyph(smileGlyph(), CHSV(hue, 180, 255),
                      CHSV(hue + 55, 150, 255),
                      slowBreath(nowMs, 190, 255, 5000),
                      gradientPhase(nowMs, 3500));
}

void renderFunSweep(SegmentDisplay& display, char rotation, uint32_t nowMs,
                    uint32_t animationStartMs) {
  const auto palette = paletteForRotation(rotation);
  const GlyphMask glyph = glyphForChar(rotation);
  display.renderGlyph(glyph, palette.dark, palette.light, 230,
                      gradientPhase(nowMs, 2600));

  const uint8_t current =
      static_cast<uint8_t>(((nowMs - animationStartMs) / 120) % 16);
  if (glyph & (1u << current)) {
    display.setSegment(static_cast<Segment>(current), palette.accent);
    display.show();
  }
}

void renderUnknown(SegmentDisplay& display, uint32_t nowMs) {
  display.renderGlyph(dashGlyph(), CRGB(150, 55, 0), CRGB(255, 175, 30),
                      slowBreath(nowMs, 140, 230, 2500),
                      gradientPhase(nowMs, 5000));
}
