#include "HardwareMidi.h"

HardwareMidi::HardwareMidi()
  : _ready(false), _onReceive(nullptr), _onRealtime(nullptr), _rxLen(0), _runningStatus(0) {}

bool HardwareMidi::isValidTxPin(int pin) {
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
  (void)pin;
  return pin >= 0;
#else
  if (pin >= 34 && pin <= 39) return false;
  return pin >= 0;
#endif
}

size_t HardwareMidi::_expectedDataBytes(uint8_t status) {
  uint8_t hi = status & 0xF0;
  if (hi == 0xC0 || hi == 0xD0) return 1;
  if (hi == 0x80 || hi == 0xA0 || hi == 0xB0 || hi == 0xE0) return 2;
  return 0;
}

void HardwareMidi::_startMessage(uint8_t status) {
  _rxBuf[0] = status;
  _rxLen = 1;
  if ((status & 0xF0) != 0xF0) {
    _runningStatus = status;
  }
}

void HardwareMidi::_appendDataByte(uint8_t b) {
  if (_rxLen >= sizeof(_rxBuf)) {
    _rxLen = 0;
    return;
  }
  _rxBuf[_rxLen++] = b;

  uint8_t status = _rxBuf[0];
  size_t need = _expectedDataBytes(status);
  if (need > 0 && _rxLen >= 1 + need) {
    _flushRxPacket();
  }
}

void HardwareMidi::_flushRxPacket() {
  if (_rxLen == 0) return;
  if (_onReceive) {
    _onReceive(_rxBuf, _rxLen);
  }
  _rxLen = 0;
}

bool HardwareMidi::begin(int rxPin, int txPin) {
  if (!isValidTxPin(txPin)) {
    _ready = false;
    return false;
  }

  Serial2.end();
  delay(10);
  Serial2.begin(31250, SERIAL_8N1, rxPin, txPin, MIDI_UART_INVERT);
  _ready = true;
  _rxLen = 0;
  _runningStatus = 0;
  return true;
}

void HardwareMidi::update() {
  if (!_ready) return;

  while (Serial2.available() > 0) {
    uint8_t b = (uint8_t)Serial2.read();

    if (b >= 0xF8) {
      if (_onRealtime) _onRealtime(b);
      continue;
    }

    if (b >= 0xF0) {
      _runningStatus = 0;
      _rxLen = 0;
      continue;
    }

    if (b >= 0x80) {
      _startMessage(b);
      if (_expectedDataBytes(b) == 0) {
        _rxLen = 0;
      }
      continue;
    }

    if (_rxLen > 0) {
      _appendDataByte(b);
      continue;
    }

    if (_runningStatus != 0 && _expectedDataBytes(_runningStatus) > 0) {
      _startMessage(_runningStatus);
      _appendDataByte(b);
    }
  }
}

void HardwareMidi::sendCC(uint8_t channel, uint8_t ccNumber, uint8_t value) {
  if (!_ready) return;
  channel = channel & 0x0F;
  ccNumber = ccNumber & 0x7F;
  value = value & 0x7F;
  uint8_t packet[3] = {
    (uint8_t)(0xB0 | channel),
    ccNumber,
    value
  };
  Serial2.write(packet, sizeof(packet));
}

void HardwareMidi::sendPC(uint8_t channel, uint8_t program) {
  if (!_ready) return;
  channel = channel & 0x0F;
  program = program & 0x7F;
  uint8_t packet[2] = {
    (uint8_t)(0xC0 | channel),
    program
  };
  Serial2.write(packet, sizeof(packet));
}
