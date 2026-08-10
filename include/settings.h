#ifndef SETTINGS_H
#define SETTINGS_H

#include "settings.pb.h"

extern SettingsMsg * const Settings;

void loadSettings();
/**
 * @return Bytes written or -1 if writing failed.
 */
int32_t saveSettings();

int32_t saveSettingsAndRestartSoon();
/**
 * @brief Call before OTA. No-op on ESP32 (Preferences/NVS has no EEPROM
 * sector rotation to pause, unlike the ESP8266 EEPROM_Rotate backend).
 */
void disableFlashPageRotation();

void nukeSettings();

#endif  // SETTINGS_H