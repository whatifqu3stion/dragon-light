# LED mapping

The display uses one continuous addressable strip routed through a single
16-segment enclosure. Segment names are written as they appear from the **front**
even though the strip was mapped from the back.

| LED | Front-facing logical segment |
|---:|---|
| 0-1 | Lower-right outer vertical |
| 2-3 | Upper-right outer vertical |
| 4-5 | Hidden / always off |
| 6 | Top-right horizontal |
| 7 | Upper-right diagonal |
| 8 | Middle-right horizontal |
| 9 | Lower-right diagonal |
| 10 | Bottom-right horizontal |
| 11-12 | Lower-center vertical |
| 13-14 | Upper-center vertical |
| 15-16 | Hidden / always off |
| 17 | Top-left horizontal |
| 18 | Upper-left diagonal |
| 19 | Middle-left horizontal |
| 20 | Lower-left diagonal |
| 21 | Bottom-left horizontal |
| 22-23 | Lower-left outer vertical |
| 24-25 | Upper-left outer vertical |

That is **26 physical LEDs**, **22 visible**, and **4 hidden travel LEDs**.
The hidden pixels let the original builder snake one continuous strand through
the enclosure instead of cutting and soldering sixteen separate pieces.

This map is still provisional. After the first USB flash, run `scan` in the
serial monitor; the firmware will light indexes `0..25` one at a time. If a
segment differs, change only the physical map in `src/display.cpp`.
