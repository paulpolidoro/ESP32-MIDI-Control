#include "MidiPresetRunner.h"
#include "Foot.h"
#include "Bluetooth.h"
#include <ArduinoJson.h>
#include <strings.h>

extern void sendCCToBle(uint8_t channel, uint8_t ccNumber, uint8_t value);

static Preferences* s_prefs = nullptr;
static Bluetooth* s_ble = nullptr;
static int s_activeSlot = 1;

enum class PresetLedMode : uint8_t { Off, On, Blink };

struct FootCfg {
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

void MidiPresetRunner::begin(Preferences& prefs, Bluetooth* ble) {
  s_prefs = &prefs;
  s_ble = ble;
  reloadFromStorage();
}

static bool typeEq(const String& a, const char* lit) {
  return strcasecmp(a.c_str(), lit) == 0;
}

bool MidiPresetRunner::executeOneLine(const String& line) {
  if (!s_ble || line.length() == 0) return false;
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
    s_ble->sendPC(ch, prog);
    return true;
  }

  if (typeEq(parts[1], "CC") && n >= 4) {
    int ccn = parts[2].toInt();
    int val = parts[3].toInt();
    if (ccn < 0 || ccn > 127 || val < 0 || val > 127) return false;
    sendCCToBle(ch, (uint8_t)ccn, (uint8_t)val);
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
  for (int i = 0; i < 4; i++) {
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
  if (err) {
    Serial.printf("[Preset] JSON erro: %s\n", err.c_str());
    return;
  }

  JsonArray feet = s_jsonDoc["feet"];
  if (feet.isNull()) return;

  for (size_t idx = 0; idx < feet.size() && idx < 4; idx++) {
    JsonObject fo = feet[idx].as<JsonObject>();
    if (fo.isNull()) continue;
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

void MidiPresetRunner::reloadFromStorage() {
  if (!s_prefs) return;
  int ap = (int)s_prefs->getUChar("active", 1);
  if (ap < 1 || ap > 10) ap = 1;
  s_activeSlot = ap;

  char key[8];
  snprintf(key, sizeof(key), "p%d", s_activeSlot);
  String j = s_prefs->getString(key, "");
  parsePresetJson(j);
  Serial.printf("[Preset] slot ativo=%d (%u bytes)\n", s_activeSlot, (unsigned)j.length());
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

void MidiPresetRunner::handleFootPress(int footId, Foot* foot) {
  if (!foot || !s_ble || footId < 0 || footId > 3) return;
  if (!s_cfg[footId].valid) return;

  FootCfg& c = s_cfg[footId];

  if (c.uniqueMode) {
    runCommandList(c.listA);
    applyFootLed(foot, c.ledA);
    return;
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
}

void MidiPresetRunner::scanIncomingBle(const uint8_t* d, size_t len) {
  if (!d || len < 2) return;
  for (size_t i = 0; i + 1 < len; i++) {
    uint8_t status = d[i];
    if ((status & 0xF0) == 0xC0) {
      uint8_t prog = d[i + 1];
      if (prog >= 1 && prog <= 10) {
        setActivePreset((int)prog);
      }
      i++;
    }
  }
}
