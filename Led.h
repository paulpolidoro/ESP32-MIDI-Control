#ifndef LED_H
#define LED_H

#include <Arduino.h>

class Led {
  public:
    Led(uint8_t pin);

    void begin();

    void on();
    void off();
    void flash(unsigned long interval = 50);

    /** Liga o LED por durationMs e apaga sozinho (não bloqueante; chamar update() no loop). */
    void pulseOnFor(unsigned long durationMs);

    bool isPulsing() const { return _pulseActive; }

    /** Brilho geral do LED: 0 a 100%. Afeta on() e o pico do flash. Padrão 100. */
    void setBrightness(uint8_t percent);
    uint8_t getBrightness() const { return _brightness; }

    void update();

  private:
    uint8_t _pin;
    uint8_t _brightness;  // 0 a 100%
    bool _state;
    bool _flashing;
    unsigned long _interval;
    unsigned long _lastToggle;
    bool _pulseActive;
    unsigned long _pulseEndMs;
};

#endif