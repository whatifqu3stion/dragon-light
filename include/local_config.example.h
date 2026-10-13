#pragma once

// Copy this file to include/local_config.h and fill in your own values.
// local_config.h is intentionally ignored by git.

#define DRAGON_LIGHT_OTA_PASSWORD "choose-a-strong-local-password"

// Password for both the temporary Dragon-Light-Setup network and the always-on
// Dragon-Light-Control hotspot. Use a strong password 8-63 characters long.
// Leaving it empty disables the direct-control hotspot.
#define DRAGON_LIGHT_SETUP_PASSWORD "choose-8-plus-characters"

// Published CSV endpoint. A Google Sheet published with ?output=csv works well.
#define DRAGON_LIGHT_SCHEDULE_URL "https://example.com/rotation.csv"

// Public iCal/ICS feed for the school calendar. Use the calendar's public
// iCal address, not its browser/embed page. Leave empty to use CSV only.
#define DRAGON_LIGHT_CALENDAR_URL "https://example.com/school-calendar.ics"
