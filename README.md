# Dragon Light

A small classroom display that answers a surprisingly common question: **what day of the rotation is it?**

My school uses an eight-day schedule spelled **B E D R A G O N**. Dragon Light turns that schedule into one illuminated character, visible at a glance. It also counts down the final minutes before class and turns off overnight.

This project combines classroom experience, physical making, and embedded software. The design aim is a useful object that can sit quietly in a room, rather than another screen competing for attention.

## Preview

### Physical prototype

<table>
  <tr>
    <td align="center" width="50%">
      <img src="docs/previews/display-wall-aday.jpg" alt="Wall-mounted Dragon Light showing a blue A" width="300">
    </td>
    <td align="center" width="50%">
      <img src="docs/previews/esp28-messy.jpg" alt="Exposed ESP8266 controller and prototype wiring behind the Dragon Light" width="300">
    </td>
  </tr>
  <tr>
    <td align="center"><sub>The wall-mounted display showing an A day.</sub></td>
    <td align="center"><sub>The ESP8266, power, and data wiring during prototype assembly.</sub></td>
  </tr>
</table>

The physical build is working, but the translucent segment faces expose an enclosure tradeoff: at close range and higher brightness, light from active LEDs can spill into neighboring channels and soften the letter. The effect is less noticeable at normal classroom viewing distance, but it reduces clarity in photographs and darker rooms.

Current mitigation combines software and hardware. The local control page provides lower, persistent brightness settings; the model part labeled as a “diffuser” works more accurately as an opaque internal baffle; and a separate thin translucent front diffuser can be added to soften individual LED hotspots. Further iteration is focused on blocking cross-segment light without losing even illumination.

### Local control interface

![Dragon Light local firmware controls with a selected display mode and example device status](docs/previews/local-controls.jpg)

The firmware's local control page, rendered with example device status. Brightness and display modes share one responsive panel, with a visible selected mode and connection feedback. The page runs directly from the device without external fonts, scripts, or services.

<details>
<summary>Mobile control interface</summary>

<p>The same controls at phone width, rendered with example device status.</p>
<img src="docs/previews/mobile-controls.jpg" alt="Dragon Light mobile controls with brightness, four display modes, and device status" width="390">

</details>

[Capture notes](docs/previews/README.md) record the source revision and demo conditions.

To explore the interface without the display, run:

```bash
python3 tools/preview_ui.py
```

Open `http://127.0.0.1:8040`. This local preview serves the exact embedded page with example status and simulated controls. Changes last only for the preview session; no hardware or calendar is connected.

## Where it stands

**Working hardware prototype.** The project notes record successful USB flashing, Wi-Fi provisioning, and basic LED rendering. The full physical LED-map scan, power-supply rating, and on-device calendar sync still need confirmation. Those checks remain open; firmware features alone are not evidence of a finished installation.

## How it works

- An ESP8266 drives a custom 16-segment display using addressable LEDs.
- A public calendar supplies the rotation letter, with a cached CSV fallback.
- A local web page controls brightness and previews animations.
- Wireless firmware updates support a display mounted out of reach.
- Missing schedule data produces an unknown indicator rather than a guessed day.

The firmware separates the physical LED routing from letters, effects, and scheduling. That makes it possible to change the enclosure or strip layout without rewriting the rest of the display.

## Explore the project

| If you want to… | Start here |
| --- | --- |
| Understand the decisions and remaining checks | [Project notes](docs/PROJECT_NOTES.md) |
| Build, configure, or troubleshoot the display | [Build and operating guide](docs/BUILD_GUIDE.md) |
| Review the wiring | [Hardware notes](docs/HARDWARE.md) |
| Understand the custom segment layout | [LED mapping](docs/LED_MAPPING.md) |
| Read the firmware | [Main application](src/main.cpp), [display mapping](src/display.cpp), [schedule handling](src/schedule.cpp) |

## Build the firmware

Requires PlatformIO. From the repository root:

```bash
pio run -e usb
```

Flashing and installation need the hardware and local configuration described in the build guide.

**Stack:** C++ · ESP8266 Arduino Core · FastLED · WiFiManager · PlatformIO.

The enclosure uses a purchased 3D model; its files are not redistributed here. Credits and installation limitations are documented in the [build guide](docs/BUILD_GUIDE.md). Project code is [MIT licensed](LICENSE).
