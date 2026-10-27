# Dragon Light

**What day of the rotation is it?**

My school uses an eight-day schedule spelled **B E D R A G O N**. Dragon Light answers that recurring question with one illuminated letter. It also counts down the final minutes before class and turns off overnight.

I wanted something useful that could sit quietly in a classroom. Building it brought together the enclosure, the light, the firmware, and the surprisingly awkward details of getting a small device onto a school network.

[Install and run Dragon Light](#install-and-run-dragon-light) · [Build guide](docs/BUILD_GUIDE.md) · [Project notes](docs/PROJECT_NOTES.md)

## The physical build

<table>
  <tr>
    <td align="center" width="50%"><img src="docs/previews/display-wall-aday.jpg" alt="Wall-mounted Dragon Light showing a blue A" width="300"></td>
    <td align="center" width="50%"><img src="docs/previews/esp28-messy.jpg" alt="ESP8266 controller and prototype wiring behind the display" width="300"></td>
  </tr>
  <tr>
    <td align="center"><sub>The display showing an A day.</sub></td>
    <td align="center"><sub>The less tidy part behind it.</sub></td>
  </tr>
</table>

**Status: working classroom prototype.** USB flashing, Wi-Fi setup, LED rendering, and calendar-based rotation have been exercised on the installed unit. Full verification of all 27 LED indexes and the power-supply ceiling is still open.

The enclosure taught me something the software couldn't solve on its own. Higher brightness lets light spill into neighboring translucent segments, softening the letter. Lower brightness helps, but the next physical iteration needs better separation: an opaque internal baffle and a thin translucent front diffuser, each doing a different job.

## The control interface

![Local firmware controls with brightness, display modes, and example device status](docs/previews/local-controls.jpg)

Brightness, display mode, and connection feedback share one panel. The page runs from the device itself without external fonts, scripts, or services. The selected mode stays visible so a person can tell what they just changed.

<details>
  <summary>Mobile control interface</summary>
  <p>The same page at phone width, with example device status.</p>
  <img src="docs/previews/mobile-controls.jpg" alt="Mobile controls with brightness, display modes, and device status" width="390">
</details>

These interface captures use example status rather than a live device connection. [Capture notes](docs/previews/README.md).

## Decisions that changed during the build

**Readability came before animation.** The normal letter no longer breathes. A brief highlight sweep adds movement without constantly changing the display, and the final-minute countdown uses a clockwise spinner. It should be easy to read from across the room.

**The existing calendar became the authority.** Maintaining another schedule would duplicate work and invite mistakes. Exact `Day X` events from the public calendar take priority, with current or cached CSV data as a fallback. When neither source can supply today's rotation, the device shows an unknown indicator instead of guessing.

**Wireless updates needed a backup.** A mounted display is inconvenient to reach, so the firmware supports password-protected over-the-air updates and local controls. The school guest network allows schedule downloads but isolates clients, blocking administration from a laptop. USB remains the reliable update and diagnostic route there. Being on the same Wi-Fi was not enough.

## How it is put together

<p align="center">
  <img src="docs/previews/dragon-light-architecture.png" alt="Calendar, CSV, and cached schedules feed a resolver; time and controls select a display state, which is mapped from logical segments to physical LEDs" width="900">
</p>

An **ESP8266** drives addressable LEDs arranged into a custom 16-segment display. The firmware separates schedule resolution, display behavior, and physical LED routing. That lets the enclosure or strip layout change without rewriting the calendar logic or letter definitions.

The schedule uses dated records rather than assuming the rotation advances every weekday. Trusted rotation data and the current time select the display state; the final mapping layer handles the unusual strip route and hidden travel pixels.

**Stack:** C++ · ESP8266 Arduino Core · FastLED · WiFiManager · PlatformIO.

[Main application](src/main.cpp) · [Display mapping](src/display.cpp) · [Schedule handling](src/schedule.cpp) · [Hardware notes](docs/HARDWARE.md) · [LED mapping](docs/LED_MAPPING.md)

## Install and run Dragon Light

### Preview the controls without hardware

You need Git and Python 3. The preview uses Python's standard library.

1. **Clone the project and open its folder.**

   ```bash
   git clone https://github.com/whatifqu3stion/dragon-light.git
   cd dragon-light
   ```

2. **Start the preview server.**

   ```bash
   python3 tools/preview_ui.py
   ```

3. **Open `http://127.0.0.1:8040`.** Try the brightness and mode controls. This serves the actual embedded page with simulated status; no hardware or calendar is connected. Changes last only for the preview session.

### Build and run the firmware

You also need PlatformIO and the hardware described in the [build guide](docs/BUILD_GUIDE.md). Run these steps from the cloned repository.

1. **Create the local configuration.**

   ```bash
   cp include/local_config.example.h include/local_config.h
   ```

   Set the update password and public schedule URLs in the copied file. Keep it local. Wi-Fi credentials are entered through the device's setup portal, not committed to the repository.

2. **Build the firmware.**

   ```bash
   pio run -e usb
   ```

3. **Follow the hardware and power checks in the build guide, then flash over USB.**

   ```bash
   pio run -e usb -t upload
   pio device monitor -b 115200
   ```

4. **Connect the device to Wi-Fi.** On first setup, join `Dragon-Light-Setup` and enter the credentials for a suitable 2.4 GHz network. Use the guide's serial diagnostics to verify the LED mapping and schedule before relying on the display.

The enclosure uses a purchased 3D model; its files are not redistributed here. Credits, wiring requirements, and network limitations are in the [build guide](docs/BUILD_GUIDE.md). Project code is [MIT licensed](LICENSE).
