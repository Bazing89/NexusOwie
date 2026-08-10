#include "settings.h"

#include <Esp.h>
#include <Preferences.h>

#include <algorithm>

#include "dprint.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "task_queue.h"

namespace {
SettingsMsg __settings = SettingsMsg_init_default;

SettingsMsg DEFAULT_SETTINGS = SettingsMsg_init_default;

// ESP32: settings are persisted as a length-prefixed protobuf blob inside
// ESP-IDF NVS via Preferences (replaces ESP8266's EEPROM_Rotate).
constexpr size_t MAX_SETTINGS_SIZE = 4096 - 2;
constexpr char kPrefsNamespace[] = "owie";
constexpr char kPrefsKey[] = "settings";

Preferences &getPrefs() {
  static Preferences prefs;
  return prefs;
}

uint8_t *settingsBuffer() {
  static uint8_t buffer[2 + MAX_SETTINGS_SIZE];
  return buffer;
}
}  // namespace

SettingsMsg * const Settings = &__settings;

void sanitizeWifiPowerSetting() {
  // check the wifi power Setting and write back a sane default if is out of
  // bounds the defined sane range is between 8dBm and 17dBm. Lower values may
  // prevent the WLAN to show up under the batterybox and a to high setting may
  // generate signal noise. defaulting to 9 brings up the WLAN with a decent
  // range of ~1-2 meters on PINTS and ~1 meter on a XR with an acceptable
  // signal strength.
  if (Settings->wifi_power < 8 || Settings->wifi_power > 17) {
    Settings->wifi_power = 9;
  }
}

void loadSettings() {
  auto &prefs = getPrefs();
  prefs.begin(kPrefsNamespace, /* readOnly = */ true);
  const size_t storedLen = prefs.getBytesLength(kPrefsKey);
  if (storedLen < 2) {
    prefs.end();
    DPRINTLN("No settings stored, resetting.");
    nukeSettings();  // nukeSettings() calls saveSettings()
    return;
  }
  uint8_t *buffer = settingsBuffer();
  prefs.getBytes(kPrefsKey, buffer, min<size_t>(storedLen, 2 + MAX_SETTINGS_SIZE));
  prefs.end();
  uint16_t len = *(uint16_t *)buffer;
  auto istream = pb_istream_from_buffer(buffer + 2,
                                        min<uint16_t>(len, MAX_SETTINGS_SIZE));
  if (pb_decode(&istream, &SettingsMsg_msg, Settings)) {
    DPRINTF("Read and decoded settings, size = %d bytes.", len);
    sanitizeWifiPowerSetting();
    return;
  }
  DPRINTLN("Failed to decode settings, resetting.");
  nukeSettings();  // nukeSettings() calls saveSettings()
}

int32_t saveSettings() {
  uint8_t *buffer = settingsBuffer();
  auto stream = pb_ostream_from_buffer(buffer + 2, MAX_SETTINGS_SIZE);
  if (!pb_encode(&stream, &SettingsMsg_msg, Settings)) {
    DPRINTLN("Failed to encode settings.");
    return -1;
  }
  *(uint16_t *)buffer = (uint16_t)stream.bytes_written;
  auto &prefs = getPrefs();
  prefs.begin(kPrefsNamespace, /* readOnly = */ false);
  prefs.putBytes(kPrefsKey, buffer, 2 + stream.bytes_written);
  prefs.end();
  DPRINTF("Serialized settings, size = %d bytes.", stream.bytes_written);
  return stream.bytes_written;
}

int32_t saveSettingsAndRestartSoon() {
  int32_t code = saveSettings();
  TaskQueue.postOneShotTask([]() { ESP.restart(); }, 1000L);
  return code;
}

void disableFlashPageRotation() {
  // No-op: Preferences/NVS has no rotating EEPROM sectors to freeze.
}

void nukeSettings() {
  *Settings = DEFAULT_SETTINGS;
  sanitizeWifiPowerSetting();
  saveSettings();
}
