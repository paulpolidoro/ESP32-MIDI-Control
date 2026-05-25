#ifndef HARDWARE_MIDI_H
#define HARDWARE_MIDI_H

#include <Arduino.h>

#ifndef MIDI_UART_INVERT
#define MIDI_UART_INVERT false
#endif

typedef void (*OnMidiReceiveCallback)(const uint8_t* data, size_t length);
typedef void (*OnMidiRealtimeCallback)(uint8_t byte);

/**
 * MIDI por cabo (UART 31250 baud).
 * ESP32 clássico: GPIO 34–39 são somente entrada — não use como TX.
 */
class HardwareMidi {
public:
  HardwareMidi();

  bool begin(int rxPin, int txPin);
  void update();

  bool isReady() const { return _ready; }

  void sendCC(uint8_t channel, uint8_t ccNumber, uint8_t value);
  void sendPC(uint8_t channel, uint8_t program);

  void setOnReceive(OnMidiReceiveCallback callback) { _onReceive = callback; }
  void setOnRealtime(OnMidiRealtimeCallback callback) { _onRealtime = callback; }

  static bool isValidTxPin(int pin);

private:
  bool _ready;
  OnMidiReceiveCallback _onReceive;
  OnMidiRealtimeCallback _onRealtime;
  uint8_t _rxBuf[16];
  size_t _rxLen;
  uint8_t _runningStatus;

  static size_t _expectedDataBytes(uint8_t status);
  void _appendDataByte(uint8_t b);
  void _startMessage(uint8_t status);
  void _flushRxPacket();
};

#endif
