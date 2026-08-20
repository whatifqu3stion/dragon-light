# Project notes

A short design record for future contributors (and future us).

## Intent

Dragon Light is a calm classroom status object built around one physical 16-segment LED character. The school's eight-day rotation spells `B E D R A G O N`, so the normal state is simply today's letter rendered with a recognizable day palette and subtle motion.

## Agreed behavior

- Rotation comes from a published CSV rather than blindly advancing weekdays.
- The schedule URL is installation-specific and stays in ignored `include/local_config.h` with Wi-Fi and OTA settings.
- `*` markers in the source calendar are ignored for now.
- Denver local time comes from NTP with automatic MST/MDT handling.
- 07:30: wake and show the day letter.
- 15:30-15:45: smile / end-of-day animation.
- 15:45-07:30: LEDs off.
- Weekends and `NONE` / `NO CLASS`: off.
- Normal letter: slow palette drift + gentle breathing.
- Every 10 minutes: one brief highlight sweep while the letter remains readable.
- Final five minutes before each class/advisory: one-digit countdown; pulse frequency increases toward start time.
- OTA updates are required because the finished unit may be mounted out of reach.

## School-day schedule

| Block | Time |
|---|---|
| 1st | 08:30-09:30 |
| 2nd | 09:35-10:35 |
| Break | 10:35-10:50 |
| 3rd | 10:55-11:55 |
| Advisory | 12:00-12:25 |
| Break / lunch | 12:30-13:20 |
| 4th | 13:25-14:25 |
| 5th | 14:30-15:30 |

Countdown windows are 08:25-08:30, 09:30-09:35, 10:50-10:55,
11:55-12:00, 13:20-13:25, and 14:25-14:30. The 12:25-12:30 move
into break is intentionally left in normal day-letter mode because it is not a
countdown to class.

## Confirmed hardware

- NodeMCU v1.0 / ESP8266, ESP-12E-family layout.
- 5V WS2812B ECO RGB strip, 60 LEDs/m.
- 26 physical pixels in the display: 22 visible and 4 hidden travel pixels.
- D1 / GPIO5 is the recommended firmware data pin.
- One shared regulated 5V supply is appropriate for NodeMCU VIN/5V + LEDs, with common ground.
- A 74AHCT125 level shifter and ~330-470 Ω series data resistor are recommended for the permanent build.

## Palette concepts

`B` cobalt/cyan · `E` forest/emerald/mint · `D` plum/violet/lavender ·
`R` burgundy/red/coral · `A` amber/gold · `G` teal/leaf/lime ·
`O` burnt orange/tangerine/peach · `N` midnight/periwinkle.

## Still to verify

1. D1/GPIO5 is the physical data connection used in the final wiring.
2. 26-index physical LED map using the `scan` command.
3. Power supply current rating and practical brightness ceiling.
