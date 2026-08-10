#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

// ESP32-C3: wrappers for WiFi APIs that don't exist under their ESP8266
// names. Include this instead of ESP8266WiFi.h.

#include <Arduino.h>
#include <WiFi.h>

// Last 16 bits of a stable per-device id for default AP SSIDs.
// Was ESP.getChipId() on ESP8266; ESP32 has no equivalent, so derive one
// from the eFuse MAC instead.
inline uint16_t owieDeviceIdSuffix() {
  return static_cast<uint16_t>(ESP.getEfuseMac() & 0xFFFF);
}

// Settings store WiFi TX power as dBm (8-17).
// Was WiFi.setOutputPower() on ESP8266; ESP32 only exposes discrete
// wifi_power_t levels via setTxPower(), so map dBm onto the nearest one.
inline void owieSetWifiTxPower(int8_t dbm) {
  if (dbm <= 0) {
    WiFi.setTxPower(WIFI_POWER_2dBm);
    return;
  }
  const int8_t clamped = dbm < 8 ? 8 : (dbm > 17 ? 17 : dbm);
  static const wifi_power_t levels[] = {
      WIFI_POWER_8_5dBm,  WIFI_POWER_11dBm,   WIFI_POWER_13dBm,
      WIFI_POWER_15dBm,   WIFI_POWER_17dBm,   WIFI_POWER_18_5dBm,
      WIFI_POWER_19dBm,   WIFI_POWER_19_5dBm, WIFI_POWER_19_5dBm,
      WIFI_POWER_19_5dBm};
  WiFi.setTxPower(levels[clamped - 8]);
}

// Was WiFi.hostname() on ESP8266; renamed to setHostname() on ESP32.
inline void owieSetWifiHostname(const char *name) { WiFi.setHostname(name); }

#endif  // PLATFORM_COMPAT_H
