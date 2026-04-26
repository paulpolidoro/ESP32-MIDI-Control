#include "TapTempo.h"
#include "Foot.h"

void TapTempo::_resetFoot(int footId) {
  if (footId < 0 || footId > 3) return;
  _tapCount[footId] = 0;
  _lastTapMs[footId] = 0;
  _bpm[footId] = 0;
  _nextBeatMs[footId] = 0;
}

void TapTempo::resetAll() {
  for (int i = 0; i < 4; i++) _resetFoot(i);
}

void TapTempo::setEnabled(int footId, bool enabled) {
  if (footId < 0 || footId > 3) return;
  if (!enabled) _resetFoot(footId);
}

bool TapTempo::onTap(int footId, Foot* foot, uint16_t* outNewBpm) {
  if (outNewBpm) *outNewBpm = 0;
  if (!foot || footId < 0 || footId > 3) return false;

  unsigned long now = millis();

  // Se o tempo "voltou" (overflow/condição estranha), reseta a contagem.
  if (_tapCount[footId] > 0 && now < _lastTapMs[footId]) {
    _tapCount[footId] = 1;
    _lastTapMs[footId] = now;
    return false;
  }

  if (_tapCount[footId] == 0) {
    _tapCount[footId] = 1;
    _lastTapMs[footId] = now;
    return false;
  }

  unsigned long delta = now - _lastTapMs[footId];
  _lastTapMs[footId] = now;
  _tapCount[footId] = (uint8_t)((_tapCount[footId] < 255) ? (_tapCount[footId] + 1) : 255);

  if (delta == 0) {
    _tapCount[footId] = 1;
    return false;
  }

  uint16_t bpm = (uint16_t)(60000UL / delta);
  if (bpm < 40 || bpm > 300) {
    _tapCount[footId] = 1;
    return false;
  }

  if (_tapCount[footId] < 2) return false;

  _bpm[footId] = bpm;
  const unsigned long periodMs = 60000UL / (unsigned long)bpm;
  _nextBeatMs[footId] = now + periodMs;
  foot->pulseLed(70);

  if (outNewBpm) *outNewBpm = bpm;
  return true;
}

void TapTempo::update(Foot* const feet[4], const bool enabledByFoot[4]) {
  if (!feet || !enabledByFoot) return;
  unsigned long now = millis();

  for (int i = 0; i < 4; i++) {
    if (!enabledByFoot[i]) continue;
    uint16_t bpm = _bpm[i];
    if (bpm == 0) continue;

    unsigned long next = _nextBeatMs[i];
    if ((long)(now - next) >= 0) {
      unsigned long periodMs = 60000UL / (unsigned long)bpm;
      feet[i]->pulseLed(70);

      if (now - next > (periodMs * 3)) {
        _nextBeatMs[i] = now + periodMs;
      } else {
        _nextBeatMs[i] = next + periodMs;
      }
    }
  }
}

uint16_t TapTempo::getBpm(int footId) const {
  if (footId < 0 || footId > 3) return 0;
  return _bpm[footId];
}

uint16_t TapTempo::getAnyBpm(const bool enabledByFoot[4]) const {
  if (!enabledByFoot) return 0;
  for (int i = 0; i < 4; i++) {
    if (enabledByFoot[i] && _bpm[i] > 0) return _bpm[i];
  }
  return 0;
}

