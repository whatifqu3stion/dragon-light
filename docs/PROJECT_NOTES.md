# Project notes

A short design record for future contributors (and future us).

## Intent

Dragon Light is a calm classroom status object built around one physical
16-segment LED character. The school's eight-day rotation spells
`B E D R A G O N`, so the normal state is simply today's letter rendered with a
recognizable day palette and subtle motion.

## Agreed behavior

- Rotation comes from a published CSV rather than blindly advancing weekdays.
- The schedule URL is installation-specific and stays in ignored
  `include/local_config.h` with Wi-Fi and OTA settings.
- `*` markers in the source calendar are ignored for now.
- Denver local time comes from NTP with automatic MST/MDT handling.
- 07:30: wake and show the day letter.
- 15:30-15:45: smile / end-of-day animation.
- 15:45-07:30: LEDs off.
- Weekends and `NONE` / `NO CLASS`: off.
- Normal letter: slow palette drift + gentle breathing.
- Every ~12-20 minutes: one short highlight sweep.
- Transitions: one-digit countdown; pulse frequency increases toward class time.
- OTA updates are required because the finished unit may be mounted out of reach.

## Palette concepts

`B` cobalt/cyan · `E` forest/emerald/mint · `D` plum/violet/lavender ·
`R` burgundy/red/coral · `A` amber/gold · `G` teal/leaf/lime ·
`O` burnt orange/tangerine/peach · `N` midnight/periwinkle.

## Still to verify

1. Exact ESP32 board model (`esp32dev` is a placeholder).
2. Actual LED data GPIO (GPIO 5 is a placeholder).
3. Exact addressable LED chipset (initial code assumes WS2812/NeoPixel timing).
4. 26-index physical LED map using the `scan` command.
5. Power supply voltage/current and safe brightness ceiling.
6. Real bell/transition windows.
