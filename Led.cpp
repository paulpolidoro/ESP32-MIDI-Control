#include "Led.h"

Led::Led(uint8_t pin)
  : _pin(pin),
    _brightness(5),
    _state(false),
    _flashing(false),
    _interval(50),
    _lastToggle(0),
    _pulseActive(false),
    _pulseEndMs(0) {}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

void Led::on() {
  _pulseActive = false;
  _flashing = false;
  _state = true;
  analogWrite(_pin, (_brightness * 255) / 100);
}

void Led::off() {
  _pulseActive = false;
  _flashing = false;
  _state = false;
  analogWrite(_pin, 0);
}

void Led::setBrightness(uint8_t percent) {
  _brightness = (percent > 100) ? 100 : percent;
  if (_state) analogWrite(_pin, (_brightness * 255) / 100);
}

void Led::flash(unsigned long interval) {
  _pulseActive = false;
  _flashing = true;
  _interval = interval;
  _lastToggle = millis();
}

void Led::pulseOnFor(unsigned long durationMs) {
  _flashing = false;
  _pulseActive = true;
  _pulseEndMs = millis() + durationMs;
  _state = true;
  analogWrite(_pin, (_brightness * 255) / 100);
}

void Led::update() {
  if (_pulseActive) {
    unsigned long now = millis();
    if ((long)(now - _pulseEndMs) >= 0) {
      _pulseActive = false;
      _state = false;
      analogWrite(_pin, 0);
    }
    return;
  }

  if (!_flashing) return;

  unsigned long now = millis();

  if (now - _lastToggle >= _interval) {
    _lastToggle = now;
    _state = !_state;
    analogWrite(_pin, _state ? ((_brightness * 255) / 100) : 0);
  }
}