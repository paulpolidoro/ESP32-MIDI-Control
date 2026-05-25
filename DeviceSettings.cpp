#include "DeviceSettings.h"
#include "Foot.h"

static Preferences* s_prefs = nullptr;
static uint8_t s_ledBrightness = DeviceSettings::DEFAULT_LED_BRIGHTNESS;
static bool s_midiClock = false;

void DeviceSettings::begin(Preferences& prefs) {
  s_prefs = &prefs;
  load();
}

void DeviceSettings::load() {
  if (!s_prefs) return;
  int b = (int)s_prefs->getUChar("ledBright", DEFAULT_LED_BRIGHTNESS);
  if (b < 0) b = 0;
  if (b > 100) b = 100;
  s_ledBrightness = (uint8_t)b;
  s_midiClock = s_prefs->getBool("midiClk", false);
}

void DeviceSettings::save() {
  if (!s_prefs) return;
  s_prefs->putUChar("ledBright", s_ledBrightness);
  s_prefs->putBool("midiClk", s_midiClock);
}

uint8_t DeviceSettings::ledBrightness() {
  return s_ledBrightness;
}

void DeviceSettings::setLedBrightness(uint8_t percent) {
  s_ledBrightness = (percent > 100) ? 100 : percent;
}

bool DeviceSettings::midiClockEnabled() {
  return s_midiClock;
}

void DeviceSettings::setMidiClockEnabled(bool enabled) {
  s_midiClock = enabled;
}

void DeviceSettings::applyLedBrightness(Foot* const feet[4]) {
  if (!feet) return;
  for (int i = 0; i < 4; i++) {
    if (feet[i]) feet[i]->setLedBrightness(s_ledBrightness);
  }
}
