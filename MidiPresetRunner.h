#ifndef MIDI_PRESET_RUNNER_H
#define MIDI_PRESET_RUNNER_H

#include <Arduino.h>
#include <Preferences.h>

class Bluetooth;
class Foot;

class MidiPresetRunner {
public:
  static void begin(Preferences& prefs, Bluetooth* ble);

  /** Chamado pelo Foot a cada borda de pressão (modo preset). */
  static bool handleFootPress(int footId, Foot* foot, uint16_t* outNewTapBpm);

  /** Chamado no loop(): mantém pisca do Tap Tempo (LED do foot). */
  static void update(Foot* const feet[4]);

  /** Varre o pacote BLE MIDI recebido: PC com programa 1–10 em qualquer canal troca o preset. */
  static void scanIncomingBle(const uint8_t* data, size_t length);

  static void setActivePreset(int preset1to10);
  static int getActivePreset();

  /** Recarrega JSON do slot ativo (p1..p10) e aplica na RAM. */
  static void reloadFromStorage();

  /** Depois de salvar via web: reparse se for o slot em uso. */
  static void notifyPresetSlotSaved(int slot1to10);

  /** UI: nome do foot (até 10 chars, pode ser vazio). */
  static const char* getFootName(int footId);
  static bool isFootTapMode(int footId);
  static uint16_t getFootTapBpm(int footId);
  static uint16_t getAnyTapBpm();  // retorna BPM do primeiro foot em tap com BPM válido (senão 0)

private:
  static void parsePresetJson(const String& json);
  static void runCommandList(const String& list);
  static bool executeOneLine(const String& line);
  static bool toggleNextIsBSide(int footId);
  static void setToggleNextIsBSide(int footId, bool nextIsBSide);
};

#endif
