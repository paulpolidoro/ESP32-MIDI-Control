// Controlador MIDI - 4 footswitches, MIDI Serial2, config BLE GATT "polimidi"

#include "Foot.h"
#include "HardwareMidi.h"
#include "ConfigBle.h"
#include "MidiPresetRunner.h"
#include "Display.h"
#include <Preferences.h>

Preferences gMidiPrefs;

static Display display(0x3C, 21, 27);
static bool displayOk = false;

static HardwareMidi hwMidi;
static ConfigBle configBle;

static void footPresetPress(int footId, Foot* foot) {
  uint16_t newBpm = 0;
  MidiPresetRunner::handleFootPress(footId, foot, &newBpm);
  (void)newBpm;
}

const int NUM_FEET = 4;
const int FOOT_PINS[NUM_FEET][2] = { {18, 19}, {4, 5}, {12, 13}, {22, 23} };

Foot foot1(0, FOOT_PINS[0][0], FOOT_PINS[0][1]);
Foot foot2(1, FOOT_PINS[1][0], FOOT_PINS[1][1]);
Foot foot3(2, FOOT_PINS[2][0], FOOT_PINS[2][1]);
Foot foot4(3, FOOT_PINS[3][0], FOOT_PINS[3][1]);
Foot* const feet[NUM_FEET] = { &foot1, &foot2, &foot3, &foot4 };

static constexpr int MIDI_RX_PIN = 34;
static constexpr int MIDI_TX_PIN = 14;

static bool anyFootTapMode() {
  return MidiPresetRunner::isFootTapMode(0) || MidiPresetRunner::isFootTapMode(1) ||
         MidiPresetRunner::isFootTapMode(2) || MidiPresetRunner::isFootTapMode(3);
}

void setup() {
  displayOk = display.begin();
  if (displayOk) {
    display.showBoot("Controlador MIDI");
    display.setConfigConnected(false);
  }

  gMidiPrefs.begin("midi_pr", false);

  hwMidi.begin(MIDI_RX_PIN, MIDI_TX_PIN);
  hwMidi.setOnReceive(MidiPresetRunner::scanIncomingMidi);
  hwMidi.setOnRealtime(MidiPresetRunner::onMidiRealtime);

  configBle.begin(ConfigBle::DEFAULT_NAME);

  MidiPresetRunner::begin(gMidiPrefs, &hwMidi);
  if (displayOk) display.setPreset((uint8_t)MidiPresetRunner::getActivePreset());

  for (int i = 0; i < NUM_FEET; i++) {
    feet[i]->begin();
    feet[i]->setOnFootPress(footPresetPress);
  }

  MidiPresetRunner::applyDeviceSettings(feet);

  if (displayOk) {
    display.showDashboard(
      MidiPresetRunner::getFootName(0),
      MidiPresetRunner::getFootName(1),
      MidiPresetRunner::getFootName(2),
      MidiPresetRunner::getFootName(3),
      anyFootTapMode(),
      MidiPresetRunner::getAnyTapBpm()
    );
  }
}

void loop() {
  for (int i = 0; i < NUM_FEET; i++) feet[i]->update();
  hwMidi.update();
  configBle.update(gMidiPrefs);
  MidiPresetRunner::update(feet);

  if (displayOk) {
    display.setConfigConnected(configBle.isConnected());
    display.update();

    static uint8_t lastPresetShown = 0;
    static uint16_t lastBpmShown = 0;
    static uint32_t lastNameSig = 0;

    uint8_t presetNow = (uint8_t)MidiPresetRunner::getActivePreset();
    bool anyTap = anyFootTapMode();
    uint16_t bpmNow = anyTap ? MidiPresetRunner::getAnyTapBpm() : 0;

    const char* a = MidiPresetRunner::getFootName(0);
    const char* b = MidiPresetRunner::getFootName(1);
    const char* c = MidiPresetRunner::getFootName(2);
    const char* d = MidiPresetRunner::getFootName(3);

    auto sigStr = [](uint32_t s, const char* p) -> uint32_t {
      if (!p) return s;
      while (*p) {
        s = (s ^ (uint8_t)*p) * 16777619u;
        p++;
      }
      return s;
    };
    uint32_t nameSig = 2166136261u;
    nameSig = sigStr(nameSig, a);
    nameSig = sigStr(nameSig, b);
    nameSig = sigStr(nameSig, c);
    nameSig = sigStr(nameSig, d);

    if (presetNow != lastPresetShown) display.setPreset(presetNow);

    if (presetNow != lastPresetShown || bpmNow != lastBpmShown || nameSig != lastNameSig) {
      display.showDashboard(a, b, c, d, anyTap, bpmNow);
      lastPresetShown = presetNow;
      lastBpmShown = bpmNow;
      lastNameSig = nameSig;
    }
  }
}
