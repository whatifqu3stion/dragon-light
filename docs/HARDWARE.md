# Hardware

## Confirmed parts

- **Controller:** NodeMCU v1.0 / ESP8266, ESP-12E-family layout
- **LEDs:** WS2812B ECO RGB strip, 5V, 60 LEDs/m
- **Display:** one 16-segment character using 27 physical pixels
- **Visible pixels:** 22
- **Hidden travel pixels:** 5 (`4`, `5`, `15`, `16`, `22`)

The current firmware uses NodeMCU **D1 / GPIO5** for LED data.

## Recommended wiring

```text
5V regulated supply
├── NodeMCU VIN / 5V
├── WS2812B +5V
└── 74AHCT125 VCC

All grounds tied together

NodeMCU D1 / GPIO5
        │ 3.3V logic
        ▼
   74AHCT125
        │ 5V logic
        ▼
   330-470 Ω
        │
        ▼
   WS2812B DIN
```

Use a **0.1 µF ceramic bypass capacitor** between VCC and GND close to the 74AHCT125.

A short 3.3V data connection often works directly with WS2812B LEDs, but the level shifter is recommended for a permanent installation because the strip itself runs at 5V.

## Power

A single 5V supply is appropriate as long as it is properly regulated and sized for the load. Do **not** feed 5V into the NodeMCU `3V3` pin; use its `VIN` / `5V` input.

With only 27 LEDs, the project does not need a huge supply. Full-white worst-case current can still be significant, so leave comfortable margin and keep firmware brightness capped. Normal classroom operation is intended to run well below maximum brightness.

## First bench test

1. Power the NodeMCU and LED strip from the shared 5V supply with common ground.
2. Connect D1/GPIO5 through the level shifter and series resistor to LED DIN.
3. Flash over USB.
4. Run `led 0`, then `scan`, and confirm indexes `0..26` against `LED_MAPPING.md`.
5. Confirm hidden pixels `4`, `5`, `15`, `16`, and `22` are physically buried and never intentionally lit by normal rendering.
6. Do not mount the unit until OTA and schedule sync have both been tested.
