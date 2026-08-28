# Project notes

A short design record for future contributors (and future us).

## Intent

Dragon Light is a calm classroom status object built around one physical 16-segment LED character. The school's eight-day rotation spells `B E D R A G O N`, so the normal state is simply today's letter rendered with a recognizable day palette and subtle motion.

## Agreed behavior

- Rotation currently comes from a published CSV rather than blindly advancing weekdays.
- A public school-calendar iCal/ICS feed is planned as the higher-priority live source once its feed URL is available.
- Intended schedule precedence: valid exact calendar `Day X` event → CSV → cached CSV; if calendar and CSV disagree, calendar wins and the mismatch is logged.
- Calendar matching must be exact: `Day B`, `Day E`, `Day D`, `Day R`, `Day A`, `Day G`, `Day O`, or `Day N`; unrelated calendar events are ignored.
- Installation-specific schedule URLs stay in ignored `include/local_config.h` with OTA/setup passwords.
- Wi-Fi SSID/password are not compiled into firmware. WiFiManager provisions them through `Dragon-Light-Setup` and stores them on the ESP8266.
- If saved Wi-Fi cannot be reached at boot, the setup portal runs for up to three minutes, then firmware continues offline/cached.
- `resetwifi` clears saved credentials and restarts provisioning.
- `*` markers in the CSV source are ignored for now.
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
- 27 physical pixels in the display: 22 visible and 5 hidden travel pixels.
- Hidden/off indexes: `4`, `5`, `15`, `16`, `22`.
- D1 / GPIO5 is the recommended firmware data pin.
- One shared regulated 5V supply is appropriate for NodeMCU VIN/5V + LEDs, with common ground.
- A 74AHCT125 level shifter and ~330-470 Ω series data resistor are recommended for the permanent build.
- ESP8266 Wi-Fi is 2.4 GHz only; simple captive-portal provisioning assumes normal SSID/password authentication, not WPA2-Enterprise/802.1X.

## 3D model source

The 3D display/enclosure geometry used for this build came from a purchased Cults3D model:
https://cults3d.com/en/orders/164606964

The model files are not redistributed in this repository and remain subject to their original license.

## Palette concepts

`B` cobalt/cyan · `E` forest/emerald/mint · `D` plum/violet/lavender ·
`R` burgundy/red/coral · `A` amber/gold · `G` teal/leaf/lime ·
`O` burnt orange/tangerine/peach · `N` midnight/periwinkle.

## Still to verify

1. D1/GPIO5 is the physical data connection used in the final wiring.
2. Powered 27-index physical LED-map verification using the `scan` command.
3. Power supply current rating and practical brightness ceiling.
4. Captive-portal provisioning against the actual installation Wi-Fi.
5. Public iCal/ICS feed URL and calendar parser behavior on-device.
