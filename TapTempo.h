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

  /** Chamar no loop: dá os "beats" para piscar LED. */
  void update(Foot* const feet[4], const bool enabledByFoot[4]);

  uint16_t getBpm(int footId) const;
  uint16_t getAnyBpm(const bool enabledByFoot[4]) const;

private:
  uint8_t _tapCount[4] = { 0, 0, 0, 0 };
  unsigned long _lastTapMs[4] = { 0, 0, 0, 0 };
  uint16_t _bpm[4] = { 0, 0, 0, 0 };
  unsigned long _nextBeatMs[4] = { 0, 0, 0, 0 };

  void _resetFoot(int footId);
};

#endif

