#ifndef FOOT_H
#define FOOT_H

#include <stddef.h>
#include <stdint.h>
#include "Led.h"

/**
 * Footswitch + LED. Configuração e envio MIDI vêm só dos presets (callback).
 * Persiste apenas o estado on/off do LED na NVS.
 */
class Foot {
public:
  Foot(int id, int buttonPin, int ledPin);

  void begin();
  void update();

  bool isOn() const { return _state; }
  bool getState() const { return _state; }

  void setState(bool on);
  void pulseLed(unsigned long ms = 500);

  typedef void (*FootPressCallback)(int footId, Foot* foot);
  void setOnFootPress(FootPressCallback callback);

private:
  int _id;
  int _buttonPin;
  Led _led;
  bool _state;
  int _lastButtonReading;
  int _lastStableState;
  unsigned long _lastDebounceTime;
  static const unsigned long DEBOUNCE_MS = 50;

  FootPressCallback _onFootPress;
  void _loadPrefs();
  void _saveState();
  void _buildKey(const char* suffix, char* out, size_t outSize) const;
  void _updateLed();
};

#endif
