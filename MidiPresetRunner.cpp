#include "MidiPresetRunner.h"
#include "Foot.h"
#include "HardwareMidi.h"
#include "DeviceSettings.h"
#include "TapTempo.h"
#include <ArduinoJson.h>
#include <strings.h>

static Preferences* s_prefs = nullptr;
static HardwareMidi* s_midi = nullptr;
static int s_activeSlot = 1;

static uint8_t s_clockTick = 0;
static unsigned long s_lastQuarterMs = 0;

static Foot* const* s_feetRef = nullptr;

static void sendTapBpmCc(uint16_t bpm) {
  if (!s_midi || bpm < 40 || bpm > 300) return;
  uint8_t cc74 = 0;
  uint8_t cc75 = 0;
  if (bpm <= 127) {
    cc75 = (uint8_t)bpm;
  } else if (bpm <= 255) {
    cc74 = 1;
    cc75 = (uint8_t)(bpm - 128);
  } else {
    cc74 = 2;
    cc75 = (uint8_t)(bpm - 256);
  }
  s_midi->sendCC(0, 74, cc74);
  s_midi->sendCC(0, 75, cc75);
}

static bool tapEnabledByFoot[4] = { false, false, false, false };

static void refreshTapEnabledFlags() {
  for (int i = 0; i < 4; i++) tapEnabledByFoot[i] = MidiPresetRunner::isFootTapMode(i);
}

enum class PresetLedMode : uint8_t { Off, On, Blink };
enum class FootMode : uint8_t { Press, TapTempo };

struct FootCfg {
  String name;
  FootMode mode;
  bool uniqueMode;
  String listA;
  String listB;
  PresetLedMode ledA;
  PresetLedMode ledB;
  bool valid;
};

static PresetLedMode parseLedMode(const char* s) {
  if (!s || !s[0]) return PresetLedMode::Off;
  if (!strcasecmp(s, "off")) return PresetLedMode::Off;
  if (!strcasecmp(s, "blink") || !strcasecmp(s, "pisca")) return PresetLedMode::Blink;
  if (!strcasecmp(s, "on")) return PresetLedMode::On;
  return PresetLedMode::Off;
}

static void applyFootLed(Foot* foot, PresetLedMode m) {
  if (!foot) return;
  switch (m) {
    case PresetLedMode::Off:
      foot->setState(false);
      break;
    case PresetLedMode::On:
      foot->setState(true);
      break;
    case PresetLedMode::Blink:
      foot->pulseLed(500);
      break;
  }
}

static FootCfg s_cfg[4];
static StaticJsonDocument<8192> s_jsonDoc;
static TapTempo s_tap;

void MidiPresetRunner::begin(Preferences& prefs, HardwareMidi* midi) {
  s_prefs = &prefs;
  s_midi = midi;
  DeviceSettings::begin(prefs);
  s_tap.setClockSyncEnabled(DeviceSettings::midiClockEnabled());
  reloadFromStorage();
}

void MidiPresetRunner::applyDeviceSettings(Foot* const feet[4]) {
  s_feetRef = feet;
  DeviceSettings::applyLedBrightness(feet);
  s_tap.setClockSyncEnabled(DeviceSettings::midiClockEnabled());
  if (!DeviceSettings::midiClockEnabled()) {
    s_tap.onClockStop();
    s_clockTick = 0;
  }
}

uint8_t MidiPresetRunner::getLedBrightness() {
  return DeviceSettings::ledBrightness();
}

bool MidiPresetRunner::isMidiClockEnabled() {
  return DeviceSettings::midiClockEnabled();
}

bool MidiPresetRunner::isClockRunning() {
  return s_tap.isClockRunning();
}

void MidiPresetRunner::saveDeviceSettings(uint8_t ledBrightness, bool midiClockEnabled) {
  DeviceSettings::setLedBrightness(ledBrightness);
  DeviceSettings::setMidiClockEnabled(midiClockEnabled);
  DeviceSettings::save();
  if (s_feetRef) applyDeviceSettings(s_feetRef);
}

void MidiPresetRunner::onMidiRealtime(uint8_t byte) {
  if (!DeviceSettings::midiClockEnabled()) return;

  if (byte == 0xFA) {
    s_clockTick = 0;
    s_lastQuarterMs = 0;
    s_tap.onClockStop();
    return;
  }
  if (byte == 0xFC) {
    s_clockTick = 0;
    s_lastQuarterMs = 0;
    s_tap.onClockStop();
    return;
  }
  if (byte == 0xFB) {
    s_clockTick = 0;
    return;
  }
  if (byte != 0xF8) return;

  s_clockTick++;
  if (s_clockTick < 24) return;

  s_clockTick = 0;
  refreshTapEnabledFlags();

  if (s_feetRef) {
    s_tap.onClockPulse(s_feetRef, tapEnabledByFoot);
  }

  unsigned long now = millis();
  if (s_lastQuarterMs == 0) {
    s_lastQuarterMs = now;
    return;
  }

  unsigned long quarterMs = now - s_lastQuarterMs;
  s_lastQuarterMs = now;
  if (quarterMs < 100 || quarterMs > 1500) return;

  uint16_t bpm = (uint16_t)(60000UL / quarterMs);
  if (bpm < 40 || bpm > 300) return;

  s_tap.setClockBpm(bpm, tapEnabledByFoot);
}

static bool typeEq(const String& a, const char* lit) {
  return strcasecmp(a.c_str(), lit) == 0;
}

bool MidiPresetRunner::executeOneLine(const String& line) {
  if (!s_midi || line.length() == 0) return false;

  String parts[6];
  int n = 0;
  int st = 0;
  while (st <= (int)line.length() && n < 6) {
    int dash = line.indexOf('-', st);
    if (dash < 0) {
      parts[n++] = line.substring(st);
      break;
    }
    parts[n++] = line.substring(st, dash);
    st = dash + 1;
  }
  if (n < 3) return false;

  int chUser = parts[0].toInt();
  if (chUser < 1 || chUser > 16) return false;
  uint8_t ch = (uint8_t)(chUser - 1) & 0x0F;

  if (typeEq(parts[1], "PC") && n >= 3) {
    int progUser = parts[2].toInt();
    uint8_t prog;
    if (progUser >= 1 && progUser <= 128) {
      prog = (uint8_t)(progUser - 1) & 0x7F;
    } else if (progUser >= 0 && progUser <= 127) {
      prog = (uint8_t)progUser;
    } else {
      return false;
    }
    s_midi->sendPC(ch, prog);
    return true;
  }

  if (typeEq(parts[1], "CC") && n >= 4) {
    int ccn = parts[2].toInt();
    int val = parts[3].toInt();
    if (ccn < 0 || ccn > 127 || val < 0 || val > 127) return false;
    s_midi->sendCC(ch, (uint8_t)ccn, (uint8_t)val);
    return true;
  }

  return false;
}

void MidiPresetRunner::runCommandList(const String& list) {
  int start = 0;
  for (;;) {
    int nl = list.indexOf('\n', start);
    String line = (nl < 0) ? list.substring(start) : list.substring(start, nl);
    line.trim();
    if (line.length() > 0) MidiPresetRunner::executeOneLine(line);
    if (nl < 0) break;
    start = nl + 1;
  }
}

void MidiPresetRunner::parsePresetJson(const String& json) {
  s_tap.resetAll();
  for (int i = 0; i < 4; i++) {
    s_cfg[i].name = "";
    s_cfg[i].mode = FootMode::Press;
    s_cfg[i].uniqueMode = true;
    s_cfg[i].listA = "";
    s_cfg[i].listB = "";
    s_cfg[i].ledA = PresetLedMode::Off;
    s_cfg[i].ledB = PresetLedMode::Off;
    s_cfg[i].valid = false;
  }

  if (json.length() < 8) return;

  s_jsonDoc.clear();
  DeserializationError err = deserializeJson(s_jsonDoc, json.c_str());
  if (err) return;

  JsonArray feet = s_jsonDoc["feet"];
  if (feet.isNull()) return;

  for (size_t idx = 0; idx < feet.size() && idx < 4; idx++) {
    JsonObject fo = feet[idx].as<JsonObject>();
    if (fo.isNull()) continue;
    const char* nm = fo["name"] | "";
    if (nm && nm[0]) {
      s_cfg[idx].name = String(nm);
      if (s_cfg[idx].name.length() > 10) s_cfg[idx].name.remove(10);
    }
    const char* mode = fo["mode"] | "press";
    s_cfg[idx].mode = (!strcasecmp(mode, "tap") || !strcasecmp(mode, "taptempo") || !strcasecmp(mode, "tap_tempo")) ? FootMode::TapTempo : FootMode::Press;
    s_tap.setEnabled((int)idx, s_cfg[idx].mode == FootMode::TapTempo);
    const char* press = fo["press"] | "unique";
    s_cfg[idx].uniqueMode = (strcmp(press, "unique") == 0);
    s_cfg[idx].listA = fo["listA"].as<String>();
    s_cfg[idx].listB = fo["listB"].as<String>();
    const char* la = fo["ledA"] | "off";
    const char* lb = fo["ledB"] | "off";
    s_cfg[idx].ledA = parseLedMode(la);
    s_cfg[idx].ledB = parseLedMode(lb);
    s_cfg[idx].valid = true;
  }
}

const char* MidiPresetRunner::getFootName(int footId) {
  if (footId < 0 || footId > 3) return "";
  return s_cfg[footId].name.c_str();
}

bool MidiPresetRunner::isFootTapMode(int footId) {
  if (footId < 0 || footId > 3) return false;
  return s_cfg[footId].valid && s_cfg[footId].mode == FootMode::TapTempo;
}

uint16_t MidiPresetRunner::getFootTapBpm(int footId) {
  if (footId < 0 || footId > 3) return 0;
  return s_tap.getBpm(footId);
}

uint16_t MidiPresetRunner::getAnyTapBpm() {
  bool enabled[4] = { isFootTapMode(0), isFootTapMode(1), isFootTapMode(2), isFootTapMode(3) };
  return s_tap.getAnyBpm(enabled);
}

void MidiPresetRunner::reloadFromStorage() {
  if (!s_prefs) return;
  int ap = (int)s_prefs->getUChar("active", 1);
  if (ap < 1 || ap > 10) ap = 1;
  s_activeSlot = ap;

  char key[8];
  snprintf(key, sizeof(key), "p%d", s_activeSlot);
  String j = s_prefs->getString(key, "");
  parsePresetJson(j);
  refreshTapEnabledFlags();
}

void MidiPresetRunner::setActivePreset(int preset1to10) {
  if (!s_prefs || preset1to10 < 1 || preset1to10 > 10) return;
  s_activeSlot = preset1to10;
  s_prefs->putUChar("active", (uint8_t)preset1to10);
  for (int i = 0; i < 4; i++) {
    char k[8];
    snprintf(k, sizeof(k), "ph%d", i);
    s_prefs->putUChar(k, 0);
  }
  reloadFromStorage();
}

int MidiPresetRunner::getActivePreset() {
  return s_activeSlot;
}

void MidiPresetRunner::notifyPresetSlotSaved(int slot1to10) {
  if (slot1to10 == s_activeSlot) reloadFromStorage();
}

bool MidiPresetRunner::toggleNextIsBSide(int footId) {
  if (!s_prefs || footId < 0 || footId > 3) return false;
  char k[8];
  snprintf(k, sizeof(k), "ph%d", footId);
  return s_prefs->getUChar(k, 0) != 0;
}

void MidiPresetRunner::setToggleNextIsBSide(int footId, bool nextIsBSide) {
  if (!s_prefs || footId < 0 || footId > 3) return;
  char k[8];
  snprintf(k, sizeof(k), "ph%d", footId);
  s_prefs->putUChar(k, nextIsBSide ? 1 : 0);
}

bool MidiPresetRunner::handleFootPress(int footId, Foot* foot, uint16_t* outNewTapBpm) {
  if (outNewTapBpm) *outNewTapBpm = 0;
  if (!foot || !s_midi || footId < 0 || footId > 3) return false;
  if (!s_cfg[footId].valid) return false;

  FootCfg& c = s_cfg[footId];

  if (c.mode == FootMode::TapTempo) {
    uint16_t bpm = 0;
    bool updated = s_tap.onTap(footId, foot, &bpm);
    if (outNewTapBpm) *outNewTapBpm = bpm;

    if (updated && bpm >= 40 && bpm <= 300) {
      sendTapBpmCc(bpm);
    }

    return updated;
  }

  if (c.uniqueMode) {
    runCommandList(c.listA);
    applyFootLed(foot, c.ledA);
    return false;
  }

  bool bSide = toggleNextIsBSide(footId);
  if (!bSide) {
    runCommandList(c.listA);
    applyFootLed(foot, c.ledA);
    setToggleNextIsBSide(footId, true);
  } else {
    runCommandList(c.listB);
    applyFootLed(foot, c.ledB);
    setToggleNextIsBSide(footId, false);
  }
  return false;
}

void MidiPresetRunner::update(Foot* const feet[4]) {
  bool enabled[4] = { isFootTapMode(0), isFootTapMode(1), isFootTapMode(2), isFootTapMode(3) };
  s_tap.update(feet, enabled);
}

void MidiPresetRunner::scanIncomingMidi(const uint8_t* d, size_t len) {
  if (!d || len < 2) return;
  if ((d[0] & 0xF0) == 0xC0) {
    uint8_t prog = d[1];
    if (prog >= 1 && prog <= 10) {
      setActivePreset((int)prog);
    }
  }
}
