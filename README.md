# Dragon Light

Dragon Light is a NodeMCU ESP8266-powered, single-character 16-segment classroom display for an eight-day rotation that spells **B E D R A G O N**.

Most of the day it quietly shows today's rotation letter in a day-specific color palette. It gets time over Wi-Fi, reads the rotation from a published schedule source, counts down class transitions, celebrates the end of the day, sleeps overnight, and supports OTA firmware updates for a display mounted out of reach.

> **Status:** first hardware pass. The controller, LED strip, and bell schedule are identified; the data connection, final LED map, and power supply still need bench verification.

## Hardware

- NodeMCU v1.0 / ESP8266 (ESP-12E-family layout)
- 5V WS2812B ECO RGB strip, 60 LEDs/m
- 26 physical pixels in this display: 22 visible + 4 hidden travel pixels
- D1 / GPIO5 is the recommended data output
- one shared regulated 5V supply can power the LEDs and NodeMCU via VIN/5V
- all grounds must be common
- recommended for a permanent build: 74AHCT125 level shifter, ~330-470 Ω data resistor, and 0.1 µF bypass capacitor at the level shifter

See [`docs/HARDWARE.md`](docs/HARDWARE.md) and [`docs/LED_MAPPING.md`](docs/LED_MAPPING.md).

## Features

- `B E D R A G O N` glyphs, digits, and a segmented smile
- distinct day palettes with slow gradient drift and gentle breathing
- brief highlight sweep every 10 minutes while the day letter remains readable
- one-digit transition countdown with increasing pulse urgency
- Denver/Mountain Time NTP clock with automatic DST handling
- published-CSV schedule sync with a LittleFS cached fallback
- weekends / `NONE` / `NO CLASS` automatically off
- 07:30 wake, 15:30 celebration, 15:45 sleep
- password-protected Arduino OTA updates
- serial diagnostics including LED-by-LED mapping

## Architecture

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

The unusual strip routing is isolated in `src/display.cpp`, so the font and effects do not care which segment uses one LED, two LEDs, or whether the strip passes through hidden pixels.

## Schedule format

The currently implemented source is a published CSV. Any stable HTTPS endpoint returning this shape will work:

```csv
date,weekday,rotation,special
2026-08-19,Wednesday,B,true
2026-08-20,Thursday,E,false
2026-09-07,Monday,NONE,false
```

`rotation` may be `B`, `E`, `D`, `R`, `A`, `G`, `O`, `N`, `NONE`, or `NO CLASS`. The `special` column is currently ignored. A Google Sheet published with `?output=csv` is convenient.

The device fetches at boot and once each school morning, then keeps the last good copy in LittleFS so a temporary Wi-Fi outage does not erase the calendar.

## Updating schedule sources

Installation-specific source URLs belong in ignored `include/local_config.h`, **not** in this public repository.

### Current source: published CSV

1. Maintain the rotation in a Google Sheet or another source that can publish raw CSV over HTTPS.
2. Keep one row per date. Use `NONE` or `NO CLASS` for dates when the display should stay dark.
3. Publish/export the sheet as CSV.
4. Set that URL in `include/local_config.h`:

```cpp
#define DRAGON_LIGHT_SCHEDULE_URL "https://example.com/rotation.csv"
```

5. Reboot the device or run `sync` over serial to force an immediate refresh. Otherwise it refreshes automatically each school morning.

Changing rows in the already-published sheet does **not** require a firmware update; the device will see the new data on its next refresh.

### Planned live cross-check: school calendar

A public school calendar can provide a stronger live source when rotation changes occur after snow days or other calendar adjustments. The intended implementation is to use a machine-readable iCal/ICS feed and accept only exact all-day event titles of the form:

```text
Day B
Day E
Day D
Day R
Day A
Day G
Day O
Day N
```

Everything else on the calendar is ignored.

Once implemented, the intended precedence is:

1. valid calendar `Day X` event for today → use it
2. otherwise use the CSV / cached CSV value
3. if both sources contain letters but disagree → use the calendar value and log the mismatch

The Google Calendar **embed URL is not the feed URL**. Use the calendar's public iCal/ICS address when adding this source. Calendar cross-checking is documented here for future setup but is **not yet implemented in firmware**.

## Setup

1. Install [PlatformIO](https://platformio.org/) and open the project.
2. Copy `include/local_config.example.h` to `include/local_config.h` and add your Wi-Fi credentials, OTA password, and published CSV URL. The real file is ignored by git.
3. Wire the LED data input to NodeMCU **D1 / GPIO5** unless you intentionally change `kLedDataPin` in `include/config.h`.
4. Flash once over USB:

```bash
pio run -e usb -t upload
pio device monitor -b 115200
```

5. Run `scan` and verify all 26 physical LED indexes.

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

After the first USB flash and a successful Wi-Fi connection, subsequent builds can be sent wirelessly. OTA is only enabled when a non-empty password is configured.

```bash
export DRAGON_LIGHT_OTA_PASSWORD='your-password'
pio run -e ota -t upload
```

The default mDNS hostname is `dragon-light.local`. Keep USB access available as a recovery path.

## Failure behavior

If time is unavailable, Dragon Light stays dark. If a schedule refresh fails, it keeps the last valid cached copy. Weekends and explicit no-class dates are dark. A missing weekday entry shows a soft amber dash rather than guessing the rotation.

## Public-repo security

No Wi-Fi credentials, OTA password, or installation-specific schedule URL belong in this repository. They live in ignored `include/local_config.h`; the sample file contains placeholders only.

The public schedule fetch currently uses an insecure TLS client to avoid hard-coding Google's changing certificate chain. That is reasonable for a non-sensitive classroom status feed, but it does not authenticate the TLS peer; use CA validation if your deployment requires stronger guarantees.

## Built with

Thanks to the open-source projects doing the heavy lifting:

- [FastLED](https://github.com/FastLED/FastLED) — addressable LED output and color tools
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino) — Wi-Fi, NTP, LittleFS, HTTPS, and OTA foundations
- [PlatformIO](https://platformio.org/) — reproducible builds, dependencies, and uploads

Dragon Light's own code is MIT licensed; see [`LICENSE`](LICENSE).

## Next steps

- verify D1/GPIO5 is the actual physical data connection
- verify the 26-index LED map with `scan`
- confirm the 5V power supply rating
- add the public iCal/ICS rotation cross-check
- tune brightness and animation intensity in the classroom

For current design decisions, see [`docs/PROJECT_NOTES.md`](docs/PROJECT_NOTES.md).
