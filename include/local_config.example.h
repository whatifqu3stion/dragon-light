#pragma once

// Copy this file to include/local_config.h and fill in your own values.
// local_config.h is intentionally ignored by git.

#define DRAGON_LIGHT_OTA_PASSWORD "choose-a-strong-local-password"

// Optional password for the temporary Dragon-Light-Setup access point.
// Leave empty for an open setup AP. If set, WiFiManager requires 8+ characters.
#define DRAGON_LIGHT_SETUP_PASSWORD "choose-8-plus-characters"

// Published CSV endpoint. A Google Sheet published with ?output=csv works well.
#define DRAGON_LIGHT_SCHEDULE_URL "https://example.com/rotation.csv"
