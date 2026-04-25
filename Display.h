#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
public:
  enum class BleState : uint8_t {
    Connecting = 0,
    Connected  = 1,
  };

  // I2C típico do SSD1306: 0x3C (às vezes 0x3D)
  explicit Display(uint8_t i2cAddress = 0x3C);
  Display(uint8_t i2cAddress, int sdaPin, int sclPin);

  // Inicia I2C + OLED. Retorna true se OK.
  bool begin(uint32_t i2cClockHz = 400000);

  void clear();
  void setInverted(bool inverted);

  // Barra de status (topo): BLE (pisca ao conectar, sólido ao conectar),
  // WiFi + IP.
  void setBleState(BleState state);
  void setWifi(bool connected, const IPAddress& ip);
  void update();  // chama no loop para piscar ícone do BLE

  // Helpers de telas comuns
  void showBoot(const char* title = "Controlador MIDI");
  void showWifiStatus(bool connected, const IPAddress& ip);
  void showPreset(uint8_t preset1to10);
  void showMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr, const char* line4 = nullptr);

private:
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 64;
  static constexpr int kResetPin = -1;  // OLED I2C normalmente sem pino RESET dedicado

  uint8_t _addr;
  int _sdaPin;
  int _sclPin;
  Adafruit_SSD1306 _oled;

  BleState _bleState = BleState::Connecting;
  bool _wifiConnected = false;
  IPAddress _wifiIp = IPAddress(0, 0, 0, 0);
  bool _bleBlinkOn = true;
  unsigned long _lastBleBlinkMs = 0;
  bool _statusDirty = true;

  void _header();             // desenha a barra de status no topo
  void _drawStatusBar(bool force);
  void _flush();
};

#endif

