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

constexpr Segment kClockSpokes[] = {
    Segment::UpperCenterV,  Segment::UpperRightDiag,
    Segment::MiddleRightH, Segment::LowerRightDiag,
    Segment::LowerCenterV, Segment::LowerLeftDiag,
    Segment::MiddleLeftH,  Segment::UpperLeftDiag,
};

constexpr Segment kOuterSegments[] = {
    Segment::TopLeftH,     Segment::TopRightH,
    Segment::UpperRightV,  Segment::LowerRightV,
    Segment::BottomRightH, Segment::BottomLeftH,
    Segment::LowerLeftV,   Segment::UpperLeftV,
};

constexpr uint8_t kBloomRing[] = {
    3, 3, 1, 1, 3, 3, 2, 2, 2, 2, 0, 0, 1, 1, 1, 1,
};

constexpr uint8_t kSymmetryGroup[] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
};

static_assert(sizeof(kClockSpokes) / sizeof(kClockSpokes[0]) == 8,
              "Clock spinner must have eight directions");
static_assert(sizeof(kBloomRing) == static_cast<uint8_t>(Segment::Count),
              "Bloom map must cover every segment");
static_assert(sizeof(kSymmetryGroup) == static_cast<uint8_t>(Segment::Count),
              "Celebration color map must cover every segment");

constexpr uint32_t kCelebrationSceneMs = 12000UL;

uint8_t sceneEnvelope(uint32_t sceneMs) {
  constexpr uint16_t kFadeMs = 500;
  if (sceneMs < kFadeMs) {
    return static_cast<uint8_t>((sceneMs * 255UL) / kFadeMs);
  }
  if (sceneMs > kCelebrationSceneMs - kFadeMs) {
    return static_cast<uint8_t>(
        ((kCelebrationSceneMs - sceneMs) * 255UL) / kFadeMs);
  }
  return 255;
}

void renderKaleidoscopeBloom(SegmentDisplay& display, uint32_t sceneMs,
                             uint32_t nowMs) {
  const uint32_t half = kCelebrationSceneMs / 2;
  const uint8_t progress = sceneMs <= half
                               ? static_cast<uint8_t>((sceneMs * 255UL) / half)
                               : static_cast<uint8_t>(
                                     ((kCelebrationSceneMs - sceneMs) * 255UL) /
                                     half);
  const uint8_t envelope = sceneEnvelope(sceneMs);
  const uint8_t baseHue = static_cast<uint8_t>(nowMs / 55);

  display.clear(false);
  for (uint8_t i = 0; i < static_cast<uint8_t>(Segment::Count); ++i) {
    const uint8_t threshold = kBloomRing[i] * 50;
    uint8_t brightness = 0;
    if (progress > threshold) {
      const uint16_t opened = static_cast<uint16_t>(progress - threshold) * 3;
      brightness = static_cast<uint8_t>(opened > 255 ? 255 : opened);
    }
    brightness = scale8(brightness, envelope);
    display.setSegment(static_cast<Segment>(i),
                       CHSV(baseHue + kSymmetryGroup[i] * 24, 190,
                            brightness));
  }
  display.show();
}

void renderFireworkBurst(SegmentDisplay& display, uint32_t sceneMs) {
  constexpr uint16_t kBurstMs = 3000;
  const uint16_t burstMs = static_cast<uint16_t>(sceneMs % kBurstMs);
  const uint8_t burstIndex = static_cast<uint8_t>(sceneMs / kBurstMs);
  const uint8_t hue = static_cast<uint8_t>(burstIndex * 61 + sceneMs / 35);
  const uint8_t envelope = sceneEnvelope(sceneMs);

  uint8_t spokeBrightness = 0;
  if (burstMs < 600) {
    spokeBrightness = static_cast<uint8_t>((burstMs * 255UL) / 600UL);
  } else if (burstMs < 1400) {
    spokeBrightness = static_cast<uint8_t>(
        ((1400UL - burstMs) * 255UL) / 800UL);
  }
  spokeBrightness = scale8(spokeBrightness, envelope);

  uint8_t sparkFade = 0;
  if (burstMs >= 500 && burstMs < 2600) {
    sparkFade = static_cast<uint8_t>(
        ((2600UL - burstMs) * 255UL) / 2100UL);
  }
  sparkFade = scale8(sparkFade, envelope);

  display.clear(false);
  for (uint8_t i = 0; i < sizeof(kClockSpokes) / sizeof(kClockSpokes[0]); ++i) {
    display.setSegment(kClockSpokes[i],
                       CHSV(hue + i * 18, 165, spokeBrightness));
  }
  for (uint8_t i = 0;
       i < sizeof(kOuterSegments) / sizeof(kOuterSegments[0]); ++i) {
    const uint8_t sparkle = sin8(static_cast<uint8_t>(
        i * 47 + burstIndex * 73 + burstMs / 5));
    const uint8_t brightness = scale8(sparkFade, 110 + scale8(sparkle, 145));
    display.setSegment(kOuterSegments[i],
                       CHSV(hue + i * 27, 130, brightness));
  }
  display.show();
}

void renderAuroraSwirl(SegmentDisplay& display, uint32_t sceneMs,
                       uint32_t nowMs) {
  const uint8_t envelope = sceneEnvelope(sceneMs);
  const uint8_t drift = static_cast<uint8_t>(nowMs / 28);

  display.clear(false);
  for (uint8_t i = 0; i < static_cast<uint8_t>(Segment::Count); ++i) {
    const uint8_t wave = sin8(static_cast<uint8_t>(drift + i * 29));
    const uint8_t brightness =
        scale8(static_cast<uint8_t>(150 + scale8(wave, 105)), envelope);
    display.setSegment(static_cast<Segment>(i),
                       CHSV(drift + i * 14 + scale8(wave, 24), 175,
                            brightness));
  }
  display.show();
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

void renderFinalMinuteSpinner(SegmentDisplay& display, char rotation,
                              uint32_t nowMs) {
  constexpr uint16_t kStepMs = 150;
  constexpr uint8_t kSpokeCount = sizeof(kClockSpokes) / sizeof(kClockSpokes[0]);

  const auto palette = paletteForRotation(rotation);
  const uint32_t step = nowMs / kStepMs;
  const uint8_t current = static_cast<uint8_t>(step % kSpokeCount);
  const uint8_t next = static_cast<uint8_t>((current + 1) % kSpokeCount);
  const uint8_t progress =
      static_cast<uint8_t>(((nowMs % kStepMs) * 255UL) / kStepMs);

  // Cross-fade neighboring center spokes to suggest a smoothly rotating clock
  // hand rather than a sequence of disconnected flashes.
  const CRGB base = blend(palette.light, palette.accent, 180);
  CRGB currentColor = base;
  CRGB nextColor = base;
  currentColor.nscale8_video(255 - progress);
  nextColor.nscale8_video(progress);

  display.clear(false);
  display.setSegment(kClockSpokes[current], currentColor);
  display.setSegment(kClockSpokes[next], nextColor);
  display.show();
}

void renderCountdown(SegmentDisplay& display, char rotation, uint8_t minutesLeft,
                     uint32_t secondsLeft, uint32_t nowMs) {
  if (secondsLeft <= 60UL) {
    renderFinalMinuteSpinner(display, rotation, nowMs);
    return;
  }

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

void renderCelebration(SegmentDisplay& display, uint32_t nowMs) {
  const uint8_t scene =
      static_cast<uint8_t>((nowMs / kCelebrationSceneMs) % 3);
  const uint32_t sceneMs = nowMs % kCelebrationSceneMs;
  if (scene == 0) {
    renderKaleidoscopeBloom(display, sceneMs, nowMs);
  } else if (scene == 1) {
    renderFireworkBurst(display, sceneMs);
  } else {
    renderAuroraSwirl(display, sceneMs, nowMs);
  }
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
