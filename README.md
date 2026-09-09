# Dragon Light

Dragon Light is a NodeMCU ESP8266-powered, single-character 16-segment classroom display for an eight-day rotation that spells **B E D R A G O N**.

Most of the day it quietly shows today's rotation letter in a day-specific color palette. It gets time over Wi-Fi, reads the rotation from a published schedule source, counts down class transitions, celebrates the end of the day, sleeps overnight, and supports OTA firmware updates for a display mounted out of reach.

> **Status:** first powered hardware pass. USB flashing, captive-portal Wi-Fi provisioning, D1/GPIO5 data, and basic LED rendering are verified. The full powered LED-map scan and power-supply rating still need verification.

## Hardware

- NodeMCU v1.0 / ESP8266 (ESP-12E-family layout)
- 5V WS2812B ECO RGB strip, 60 LEDs/m
- 27 physical pixels in this display: 22 visible + 5 hidden travel pixels
- hidden/off pixel indexes: `4`, `5`, `15`, `16`, `22`
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
- public iCal/ICS live calendar with a LittleFS-cached CSV fallback
- weekends / `NONE` / `NO CLASS` automatically off
- 07:30 wake, 15:30 celebration, 15:45 sleep
- captive-portal Wi-Fi setup with credentials stored on the device
- password-protected Arduino OTA updates
- serial diagnostics including LED-by-LED mapping and Wi-Fi reset

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

## Schedule sources

Dragon Light checks two independent public sources. A valid calendar event is
the live authority; the published CSV remains the durable fallback.

### Published CSV fallback

The fallback source is a published CSV. Any stable HTTPS endpoint returning this shape will work:

```csv
date,weekday,rotation,special
2026-08-19,Wednesday,B,true
2026-08-20,Thursday,E,false
2026-09-07,Monday,NONE,false
```

`rotation` may be `B`, `E`, `D`, `R`, `A`, `G`, `O`, `N`, `NONE`, or `NO CLASS`. The `special` column is currently ignored. A Google Sheet published with `?output=csv` is convenient.

The CSV parser locates columns by header name, so an extra exported `index`
column is harmless. The device fetches at boot and once each school morning,
then keeps the last good copy in LittleFS so a temporary Wi-Fi outage does not
erase the schedule.

### Live school calendar

The calendar source must be a public machine-readable iCal/ICS feed. Dragon
Light accepts only exact, all-day event titles:

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

Timed events, differently named events, and unrelated calendar entries are
ignored. If conflicting Day events exist on one date, Dragon Light logs the
conflict and safely falls back to CSV.

Schedule precedence is:

1. valid exact calendar `Day X` event for today
2. current or cached CSV value
3. amber unknown dash when neither source has today

If the calendar and CSV contain different letters, the calendar wins and the
mismatch is logged once.

## Updating schedule sources

Installation-specific source URLs belong in ignored `include/local_config.h`, **not** in this public repository.

### Configure the published CSV

1. Maintain the rotation in a Google Sheet or another source that can publish raw CSV over HTTPS.
2. Keep one row per date. Use `NONE` or `NO CLASS` for dates when the display should stay dark.
3. Publish/export the sheet as CSV.
4. Set that URL in `include/local_config.h`:

```cpp
#define DRAGON_LIGHT_SCHEDULE_URL "https://example.com/rotation.csv"
```

For Google Sheets, the published URL should end like
`/pub?gid=123456789&single=true&output=csv`. A `/pubhtml` link is the human
viewing page and will not work as the firmware's CSV source.

5. Reboot the device or run `sync` over serial to force an immediate refresh. Otherwise it refreshes automatically each school morning.

Changing rows in the already-published sheet does **not** require a firmware update; the device will see the new data on its next refresh.

### Configure the live calendar

1. Make the relevant school calendar publicly readable.
2. In Google Calendar, open **Settings and sharing → Integrate calendar**.
3. Copy **Public address in iCal format**. Do not use the normal calendar page,
   embed URL, or a private/secret feed in this public-device configuration.
4. Set the address in `include/local_config.h`:

```cpp
#define DRAGON_LIGHT_CALENDAR_URL "https://example.com/school-calendar.ics"
```

5. Reboot or run `sync`. Use `status` to see `source=calendar`, `source=csv`,
   or `source=none` for today's resolved letter.

Changing events in either already-published source does not require another
firmware upload.

## First setup

1. Install [PlatformIO](https://platformio.org/) and open the project.
2. Copy `include/local_config.example.h` to `include/local_config.h`. Set the OTA password, published CSV URL, public iCal/ICS URL, and optionally a private password for the temporary setup access point. **Wi-Fi SSID/password do not go in this file.**
3. Wire the LED data input to NodeMCU **D1 / GPIO5** unless you intentionally change `kLedDataPin` in `include/config.h`.
4. Flash once over USB:

```bash
pio run -e usb -t upload
pio device monitor -b 115200
```

5. On first boot, Dragon Light tries any Wi-Fi credentials already stored on the ESP8266. If none work, it creates a temporary network named **`Dragon-Light-Setup`**.
6. Connect a phone/laptop to `Dragon-Light-Setup`, choose the desired 2.4 GHz Wi-Fi network, enter its password, and save. The captive portal usually opens automatically; if it does not, browse to `192.168.4.1`.
7. The ESP8266 stores those Wi-Fi credentials in its own flash and reconnects automatically on later boots. They are not compiled into the firmware or committed to GitHub.
8. Run `scan` and verify all 27 physical LED indexes (`0..26`).

The setup portal waits for up to three minutes, then the firmware continues with any cached schedule it has. On a later reboot, it will try setup again if no saved network works.

**Wi-Fi compatibility:** ESP8266 supports 2.4 GHz Wi-Fi. This setup flow is intended for normal SSID/password networks. WPA2-Enterprise / 802.1X school networks require different authentication code or a suitable guest/IoT network.

Useful serial commands:

```text
auto       normal scheduled behavior
scan       cycle through every physical LED
led 7      light one physical LED
glyph B    show a glyph
smile      show the segmented smile
off        force LEDs off
sync       refresh time + schedule now
resetwifi  erase saved Wi-Fi and restart setup
status     print current state
help       list commands
```

## Changing Wi-Fi later

If the network name or password changes, send `resetwifi` over the serial monitor. Dragon Light clears the stored credentials, restarts, and broadcasts `Dragon-Light-Setup` again.

If the old network simply becomes unavailable, a reboot also triggers the setup portal automatically after the saved connection attempt fails. This means the finished unit does not normally need to be removed from the wall just to change Wi-Fi.

## OTA updates

After the first USB flash and a successful Wi-Fi connection, subsequent builds can be sent wirelessly. OTA is only enabled when a non-empty password is configured.

```bash
export DRAGON_LIGHT_OTA_PASSWORD='your-password'
pio run -e ota -t upload
```

The default mDNS hostname is `dragon-light.local`. Keep USB access available as a recovery path.

## Failure behavior

If time is unavailable, Dragon Light stays dark. If the live calendar is absent,
incomplete, conflicting, or temporarily unavailable, Dragon Light uses the
current or cached CSV. Weekends and explicit CSV no-class dates are dark. A
date missing from both sources shows a soft amber dash rather than guessing the
rotation. If saved Wi-Fi cannot be reached, the device offers its setup portal
before continuing offline.

## Public-repo security

Wi-Fi credentials are provisioned through the local captive portal and stored on the ESP8266, not in this repository. OTA/setup passwords and installation-specific CSV/calendar URLs live in ignored `include/local_config.h`; the sample file contains placeholders only.

The public schedule fetch currently uses an insecure TLS client to avoid hard-coding Google's changing certificate chain. That is reasonable for a non-sensitive classroom status feed, but it does not authenticate the TLS peer; use CA validation if your deployment requires stronger guarantees.

## Built with and credits

Thanks to the open-source projects doing the heavy lifting:

- [FastLED](https://github.com/FastLED/FastLED) — addressable LED output and color tools
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino) — Wi-Fi, NTP, LittleFS, HTTPS, and OTA foundations
- [WiFiManager](https://github.com/tzapu/WiFiManager) — captive-portal Wi-Fi provisioning
- [PlatformIO](https://platformio.org/) — reproducible builds, dependencies, and uploads

The 3D display/enclosure geometry used for this specific build came from a purchased [Cults3D model](https://cults3d.com/en/orders/164606964). The 3D model files are **not** redistributed by this repository and remain under their original license.

Dragon Light's own code is MIT licensed; see [`LICENSE`](LICENSE).

## Next steps

- verify the 27-index LED map with `scan`
- confirm the 5V power supply rating
- validate the public iCal/ICS feed against real school-calendar events
- tune brightness and animation intensity in the classroom

For current design decisions, see [`docs/PROJECT_NOTES.md`](docs/PROJECT_NOTES.md).
