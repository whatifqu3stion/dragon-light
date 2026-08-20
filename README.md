# Dragon Light

Dragon Light is an ESP32-powered, single-character 16-segment classroom display
for an eight-day rotation that spells **B E D R A G O N**.

Most of the day it quietly shows today's rotation letter in a day-specific color
palette. It gets time over Wi-Fi, reads the rotation from a published CSV,
briefly counts down class transitions, celebrates the end of the day, sleeps
overnight, and supports OTA firmware updates for a display mounted out of reach.

> **Status:** first hardware pass. The architecture is usable, but the exact
> ESP32 board, data GPIO, LED chipset, bell schedule, and final LED map still need
> verification on the physical unit.

## Features

- `B E D R A G O N` glyphs plus digits and a segmented smile icon
- distinct day palettes with slow gradient drift and gentle breathing
- occasional short highlight animation rather than constant motion
- one-digit transition countdown with increasing pulse urgency
- NTP clock with Denver/Mountain Time DST handling
- published-CSV schedule sync with a LittleFS cached fallback
- weekends / `NONE` / `NO CLASS` automatically off
- 07:30 wake, 15:30 celebration, 15:45 sleep (easy to change)
- password-protected Arduino OTA updates
- serial diagnostics including LED-by-LED mapping

## How it is organized

```text
physical LED indexes
        ↓
16 logical segments
        ↓
letters / numbers / icons
        ↓
palettes + effects
        ↓
time + rotation + transition state
```

The unusual strip routing is isolated in `src/display.cpp`, so fonts and effects
do not care that some segments use two LEDs or that four physical LEDs are
hidden travel pixels. See [`docs/LED_MAPPING.md`](docs/LED_MAPPING.md).

## Hardware assumptions

The current physical map is **26 addressable LEDs**: 22 visible and 4 hidden.
The first pass assumes a generic ESP32, WS2812/NeoPixel-style pixels, and GPIO 5
for data. **Those three assumptions are provisional. Verify them before a final
installation.**

## Schedule format

Any stable HTTPS endpoint that returns this CSV shape will work:

```csv
date,weekday,rotation,special
2026-08-19,Wednesday,B,true
2026-08-20,Thursday,E,false
2026-09-07,Monday,NONE,false
```

`rotation` may be `B`, `E`, `D`, `R`, `A`, `G`, `O`, `N`, `NONE`, or
`NO CLASS`. The `special` column is currently ignored. A Google Sheet published
with `?output=csv` is a convenient source.

The device fetches at boot and once each school morning, then keeps the last good
copy in LittleFS so a temporary Wi-Fi outage does not erase the calendar.

## Setup

1. Install [PlatformIO](https://platformio.org/) and open the project.
2. Confirm the board in `platformio.ini` and the LED pin/count in
   `include/config.h`.
3. Copy `include/local_config.example.h` to `include/local_config.h` and add your
   Wi-Fi credentials, OTA password, and published CSV URL. The real file is
   ignored by git.
4. Flash once over USB:

```bash
pio run -e usb -t upload
pio device monitor -b 115200
```

5. Run `scan` in the serial monitor and verify all 26 physical LED indexes.

Useful serial commands:

```text
auto       normal scheduled behavior
scan       cycle through every physical LED
led 7      light one physical LED
glyph B    show a glyph
smile      show the segmented smile
off        force LEDs off
sync       refresh time + schedule now
status     print current state
help       list commands
```

## OTA updates

After the first USB flash and a successful Wi-Fi connection, subsequent builds
can be sent wirelessly. The firmware only enables OTA when a non-empty OTA
password is configured.

Set the same password in your shell, then use the PlatformIO `ota` environment:

```bash
export DRAGON_LIGHT_OTA_PASSWORD='your-password'
pio run -e ota -t upload
```

The default mDNS hostname is `dragon-light.local`. Keep USB access as a recovery
path in case Wi-Fi settings are ever broken.

## Failure behavior

Dragon Light is deliberately conservative: if time is unavailable it stays dark;
if a schedule refresh fails it keeps the last valid cached copy; weekends and
explicit no-class dates are dark; and a missing weekday entry shows a soft amber
dash instead of guessing the rotation.

## Security / public-repo note

No Wi-Fi credentials, OTA password, or installation-specific schedule URL belong
in this repository. They live in ignored `include/local_config.h`. The sample
file contains placeholders only.

The public schedule fetch currently uses `WiFiClientSecure::setInsecure()` to
avoid hard-coding Google's changing certificate chain. That is acceptable for a
non-sensitive classroom status feed, but it does not authenticate the TLS peer;
replace it with CA validation if your deployment needs stronger guarantees.

## Built with

Thanks to the open-source projects doing the heavy lifting:

- [FastLED](https://github.com/FastLED/FastLED) — addressable LED output and color tools
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32) — ESP32 Arduino framework, Wi-Fi, NTP, LittleFS and OTA foundations
- [PlatformIO](https://platformio.org/) — reproducible builds, dependencies and upload workflow

Dragon Light's own code is MIT licensed; see [`LICENSE`](LICENSE).

## Next hardware steps

- confirm ESP32 model and LED chipset
- confirm data GPIO and power supply
- verify the 26-LED map with `scan`
- add the real bell/transition windows
- tune brightness and animation intensity in the classroom

For the current design decisions, see [`docs/PROJECT_NOTES.md`](docs/PROJECT_NOTES.md).
