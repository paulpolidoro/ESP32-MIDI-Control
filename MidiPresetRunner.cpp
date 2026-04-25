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

static uint8_t s_tapCount[4] = { 0, 0, 0, 0 };
static unsigned long s_lastTapMs[4] = { 0, 0, 0, 0 };
static uint16_t s_tapBpm[4] = { 0, 0, 0, 0 };
static unsigned long s_nextBeatMs[4] = { 0, 0, 0, 0 };

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
    s_cfg[i].name = "";
    s_cfg[i].mode = FootMode::Press;
    s_cfg[i].uniqueMode = true;
    s_cfg[i].listA = "";
    s_cfg[i].listB = "";
    s_cfg[i].ledA = PresetLedMode::Off;
    s_cfg[i].ledB = PresetLedMode::Off;
    s_cfg[i].valid = false;

    s_tapCount[i] = 0;
    s_lastTapMs[i] = 0;
    s_tapBpm[i] = 0;
    s_nextBeatMs[i] = 0;
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
    const char* nm = fo["name"] | "";
    if (nm && nm[0]) {
      s_cfg[idx].name = String(nm);
      if (s_cfg[idx].name.length() > 10) s_cfg[idx].name.remove(10);
    }
    const char* mode = fo["mode"] | "press";
    s_cfg[idx].mode = (!strcasecmp(mode, "tap") || !strcasecmp(mode, "taptempo") || !strcasecmp(mode, "tap_tempo")) ? FootMode::TapTempo : FootMode::Press;
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
  return s_tapBpm[footId];
}

uint16_t MidiPresetRunner::getAnyTapBpm() {
  for (int i = 0; i < 4; i++) {
    if (isFootTapMode(i) && s_tapBpm[i] > 0) return s_tapBpm[i];
  }
  return 0;
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

static bool handleTapTempoPress(int footId, Foot* foot, uint16_t* outNewTapBpm) {
  if (outNewTapBpm) *outNewTapBpm = 0;
  if (!foot || footId < 0 || footId > 3) return false;

  unsigned long now = millis();

  // Se o tempo "voltou" (overflow/condição estranha), reseta a contagem.
  if (s_tapCount[footId] > 0 && now < s_lastTapMs[footId]) {
    s_tapCount[footId] = 1;
    s_lastTapMs[footId] = now;
    return false;
  }

  if (s_tapCount[footId] == 0) {
    s_tapCount[footId] = 1;
    s_lastTapMs[footId] = now;
    return false;
  }

  unsigned long delta = now - s_lastTapMs[footId];
  s_lastTapMs[footId] = now;
  s_tapCount[footId] = (uint8_t)((s_tapCount[footId] < 255) ? (s_tapCount[footId] + 1) : 255);

  if (delta == 0) {
    s_tapCount[footId] = 1;
    return false;
  }

  // Converte para BPM e valida a faixa pedida (40..250).
  uint16_t bpm = (uint16_t)(60000UL / delta);
  if (bpm < 40 || bpm > 250) {
    s_tapCount[footId] = 1;
    return false;
  }

  // Precisa de no mínimo 2 taps para "gerar" BPM.
  if (s_tapCount[footId] < 2) return false;

  s_tapBpm[footId] = bpm;
  const unsigned long periodMs = 60000UL / (unsigned long)bpm;
  s_nextBeatMs[footId] = now + periodMs;
  foot->pulseLed(70);

  if (outNewTapBpm) *outNewTapBpm = bpm;
  return true;
}

bool MidiPresetRunner::handleFootPress(int footId, Foot* foot, uint16_t* outNewTapBpm) {
  if (outNewTapBpm) *outNewTapBpm = 0;
  if (!foot || !s_ble || footId < 0 || footId > 3) return false;
  if (!s_cfg[footId].valid) return false;

  FootCfg& c = s_cfg[footId];

  if (c.mode == FootMode::TapTempo) {
    return handleTapTempoPress(footId, foot, outNewTapBpm);
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
  if (!feet) return;
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    if (!s_cfg[i].valid) continue;
    if (s_cfg[i].mode != FootMode::TapTempo) continue;
    uint16_t bpm = s_tapBpm[i];
    if (bpm == 0) continue;

    unsigned long next = s_nextBeatMs[i];
    if ((long)(now - next) >= 0) {
      unsigned long periodMs = 60000UL / (unsigned long)bpm;
      feet[i]->pulseLed(70);

      // Se atrasou muito (ex.: loop travou), reancora para frente.
      if (now - next > (periodMs * 3)) {
        s_nextBeatMs[i] = now + periodMs;
      } else {
        s_nextBeatMs[i] = next + periodMs;
      }
    }
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
