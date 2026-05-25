#ifndef DEVICE_SETTINGS_H
#define DEVICE_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

class Foot;

class DeviceSettings {
public:
  static constexpr uint8_t DEFAULT_LED_BRIGHTNESS = 80;

  static void begin(Preferences& prefs);
  static void load();
  static void save();

  static uint8_t ledBrightness();
  static void setLedBrightness(uint8_t percent);

  static bool midiClockEnabled();
  static void setMidiClockEnabled(bool enabled);

  static void applyLedBrightness(Foot* const feet[4]);
};

#endif
