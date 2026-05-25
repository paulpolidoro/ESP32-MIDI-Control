#ifndef TAP_TEMPO_H
#define TAP_TEMPO_H

#include <Arduino.h>

class Foot;

class TapTempo {
public:
  TapTempo() = default;

  void resetAll();
  void setEnabled(int footId, bool enabled);

  /** Retorna true se gerou um BPM novo (>=2 taps, faixa válida). */
  bool onTap(int footId, Foot* foot, uint16_t* outNewBpm);

  /** Chamar no loop: dá os "beats" para piscar LED (ignorado se clock MIDI ativo). */
  void update(Foot* const feet[4], const bool enabledByFoot[4]);

  /** Sincronização via MIDI Clock (24 ticks = 1 semínima). */
  void setClockSyncEnabled(bool enabled);
  bool isClockSyncEnabled() const { return _clockSync; }
  /** Pulsa a cada semínima do MIDI Clock (24 ticks). */
  void onClockPulse(Foot* const feet[4], const bool enabledByFoot[4]);
  void setClockBpm(uint16_t bpm, const bool enabledByFoot[4]);
  void onClockStop();

  uint16_t getBpm(int footId) const;
  uint16_t getAnyBpm(const bool enabledByFoot[4]) const;
  uint16_t getClockBpm() const { return _clockBpm; }
  bool isClockRunning() const { return _clockRunning; }

private:
  bool _clockSync = false;
  bool _clockRunning = false;
  uint16_t _clockBpm = 0;
  uint8_t _tapCount[4] = { 0, 0, 0, 0 };
  unsigned long _lastTapMs[4] = { 0, 0, 0, 0 };
  uint16_t _bpm[4] = { 0, 0, 0, 0 };
  unsigned long _nextBeatMs[4] = { 0, 0, 0, 0 };

  void _resetFoot(int footId);
};

#endif

