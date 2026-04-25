#include "Foot.h"
#include <Arduino.h>
#include <Preferences.h>

static const char* PREF_NAMESPACE = "foot";

Foot::Foot(int id, int buttonPin, int ledPin)
  : _id(id),
    _buttonPin(buttonPin),
    _led((uint8_t)ledPin),
    _state(false),
    _lastButtonReading(LOW),
    _lastStableState(LOW),
    _lastDebounceTime(0),
    _onFootPress(nullptr) {
}

void Foot::_buildKey(const char* suffix, char* out, size_t outSize) const {
  snprintf(out, outSize, "%d_%s", _id, suffix);
}

void Foot::_loadPrefs() {
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) return;
  char key[16];
  _buildKey("state", key, sizeof(key));
  if (prefs.isKey(key)) {
    _state = prefs.getBool(key, false);
  }
  prefs.end();
}

void Foot::_saveState() {
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) return;
  char key[16];
  _buildKey("state", key, sizeof(key));
  prefs.putBool(key, _state);
  prefs.end();
}

void Foot::_updateLed() {
  if (_led.isPulsing()) return;
  if (_state) _led.on();
  else _led.off();
}

void Foot::begin() {
  pinMode(_buttonPin, INPUT_PULLUP);
  _led.begin();
  _loadPrefs();
  _lastStableState = digitalRead(_buttonPin);
  _lastButtonReading = _lastStableState;
  _updateLed();
}

void Foot::update() {
  _led.update();

  int reading = digitalRead(_buttonPin);

  if (reading != _lastButtonReading) {
    _lastDebounceTime = millis();
  }
  _lastButtonReading = reading;

  if ((millis() - _lastDebounceTime) >= DEBOUNCE_MS) {
    if (reading == LOW && _lastStableState == HIGH) {
      if (_onFootPress) {
        _onFootPress(_id, this);
      }
    }
    _lastStableState = reading;
  }
}

void Foot::setState(bool on) {
  _state = on;
  _updateLed();
  _saveState();
}

void Foot::pulseLed(unsigned long ms) {
  _state = false;
  _saveState();
  _led.pulseOnFor(ms);
}

void Foot::setOnFootPress(FootPressCallback callback) {
  _onFootPress = callback;
  _updateLed();
}
