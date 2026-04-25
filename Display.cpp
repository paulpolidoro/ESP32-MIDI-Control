#include "Display.h"

static constexpr uint8_t kIconBt8x8[] PROGMEM = {
  0b00011000,
  0b00010100,
  0b01010010,
  0b00101100,
  0b00101100,
  0b01010010,
  0b00010100,
  0b00011000,
};

static constexpr uint8_t kIconWifi8x8[] PROGMEM = {
  0b00111100,
  0b01000010,
  0b00011000,
  0b00100100,
  0b00000000,
  0b00011000,
  0b00011000,
  0b00000000,
};

Display::Display(uint8_t i2cAddress)
  : _addr(i2cAddress),
    _sdaPin(-1),
    _sclPin(-1),
    _oled(kWidth, kHeight, &Wire, kResetPin) {
}

Display::Display(uint8_t i2cAddress, int sdaPin, int sclPin)
  : _addr(i2cAddress),
    _sdaPin(sdaPin),
    _sclPin(sclPin),
    _oled(kWidth, kHeight, &Wire, kResetPin) {
}

bool Display::begin(uint32_t i2cClockHz) {
  if (_sdaPin >= 0 && _sclPin >= 0) {
    Wire.begin(_sdaPin, _sclPin);
  } else {
    Wire.begin();
  }
  Wire.setClock(i2cClockHz);

  if (!_oled.begin(SSD1306_SWITCHCAPVCC, _addr)) {
    return false;
  }

  _oled.clearDisplay();
  _oled.setTextColor(SSD1306_WHITE);
  _oled.setTextSize(1);
  _oled.setCursor(0, 0);
  _oled.display();
  return true;
}

void Display::clear() {
  _oled.clearDisplay();
  _oled.setCursor(0, 0);
  _oled.display();
}

void Display::setInverted(bool inverted) {
  _oled.invertDisplay(inverted);
}

void Display::setBleState(BleState state) {
  if (_bleState == state) return;
  _bleState = state;
  _statusDirty = true;
}

void Display::setWifi(bool connected, const IPAddress& ip) {
  if (_wifiConnected == connected && _wifiIp == ip) return;
  _wifiConnected = connected;
  _wifiIp = ip;
  _statusDirty = true;
}

void Display::setPreset(uint8_t preset1to10) {
  if (preset1to10 < 1) preset1to10 = 1;
  if (preset1to10 > 10) preset1to10 = 10;
  if (_preset == preset1to10) return;
  _preset = preset1to10;
  _statusDirty = true;
}

void Display::update() {
  const unsigned long kBlinkMs = 350;
  if (_bleState == BleState::Connecting) {
    if (millis() - _lastBleBlinkMs > kBlinkMs) {
      _lastBleBlinkMs = millis();
      _bleBlinkOn = !_bleBlinkOn;
      _statusDirty = true;
    }
  } else {
    if (!_bleBlinkOn) {
      _bleBlinkOn = true;
      _statusDirty = true;
    }
  }

  _drawStatusBar(false);
}

void Display::_header() {
  _oled.clearDisplay();
  _oled.setTextSize(1);
  _oled.setTextColor(SSD1306_WHITE);
  _drawStatusBar(true);
  _oled.drawLine(0, 11, 127, 11, SSD1306_WHITE);
}

void Display::_drawStatusBar(bool force) {
  if (!_statusDirty && !force) return;
  _statusDirty = false;

  // Limpa só a faixa do topo (0..10) pra não apagar o conteúdo abaixo.
  _oled.fillRect(0, 0, kWidth, 11, SSD1306_BLACK);

  // BLE icon (x=0)
  if (_bleState == BleState::Connected || _bleBlinkOn) {
    _oled.drawBitmap(0, 1, kIconBt8x8, 8, 8, SSD1306_WHITE);
  }

  // WiFi + IP justificado à direita
  char ipBuf[16];
  if (_wifiConnected) {
    snprintf(ipBuf, sizeof(ipBuf), "%u.%u.%u.%u", _wifiIp[0], _wifiIp[1], _wifiIp[2], _wifiIp[3]);
  } else {
    snprintf(ipBuf, sizeof(ipBuf), "--.--.--.--");
  }

  int16_t x1, y1;
  uint16_t w, h;
  _oled.getTextBounds(ipBuf, 0, 0, &x1, &y1, &w, &h);

  const int ipY = 1;
  const int iconW = 8;
  const int gap = 2;
  const int rightPad = 1;

  int ipX = (int)kWidth - rightPad - (int)w;
  if (ipX < 20) ipX = 20;  // evita invadir o ícone do BLE

  int iconX = ipX - gap - iconW;
  if (iconX < 10) iconX = 10;

  // Preset depois do Bluetooth: " P{n}"
  char pbuf[6];
  snprintf(pbuf, sizeof(pbuf), " P%u", (unsigned)_preset);
  _oled.setCursor(10, 1);
  _oled.setTextSize(1);
  _oled.setTextColor(SSD1306_WHITE);
  _oled.getTextBounds(pbuf, 10, 1, &x1, &y1, &w, &h);
  int presetRight = 10 + (int)w;
  if (presetRight < iconX - 2) {
    _oled.print(pbuf);
  }

  if (_wifiConnected) {
    _oled.drawBitmap(iconX, 1, kIconWifi8x8, 8, 8, SSD1306_WHITE);
  } else {
    _oled.drawRect(iconX, 1, 8, 8, SSD1306_WHITE);
  }

  _oled.setCursor(ipX, ipY);
  _oled.print(ipBuf);
}

void Display::_flush() {
  _oled.display();
}

void Display::showBoot(const char* title) {
  (void)title;
  _header();
  _oled.setCursor(0, 16);
  _oled.print("Iniciando...");
  _flush();
}

void Display::showWifiStatus(bool connected, const IPAddress& ip) {
  setWifi(connected, ip);
  _header();
  _oled.setCursor(0, 16);
  _oled.print("Status: ");
  _oled.print(connected ? "conectado" : "desconectado");

  _oled.setCursor(0, 28);
  _oled.print("IP: ");
  if (connected) {
    _oled.print(ip);
  } else {
    _oled.print("-");
  }
  _flush();
}

void Display::showPreset(uint8_t preset1to10) {
  _header();
  _oled.setCursor(0, 20);
  _oled.setTextSize(2);
  _oled.print((int)preset1to10);
  _oled.setTextSize(1);
  _flush();
}

void Display::showBpm(uint16_t bpm) {
  _header();

  char buf[8];
  snprintf(buf, sizeof(buf), "%u", (unsigned)bpm);

  _oled.setTextSize(1);
  _oled.setCursor(0, 16);
  _oled.print("BPM");

  int16_t x1, y1;
  uint16_t w, h;
  _oled.setTextSize(3);
  _oled.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  int x = (kWidth - (int)w) / 2;
  if (x < 0) x = 0;
  _oled.setCursor(x, 28);
  _oled.print(buf);

  _oled.setTextSize(1);
  _flush();
}

static const char* fallbackFootName(const char* s, const char* fallback) {
  if (!s) return fallback;
  while (*s == ' ') s++;
  if (!*s) return fallback;
  return s;
}

void Display::showDashboard(const char* footA, const char* footB, const char* footC, const char* footD, bool tapActive, uint16_t bpmOr0) {
  _header();

  // Área abaixo da barra (y=12..63)
  _oled.fillRect(0, 12, kWidth, kHeight - 12, SSD1306_BLACK);

  const char* a = fallbackFootName(footA, "Foot A");
  const char* b = fallbackFootName(footB, "Foot B");
  const char* c = fallbackFootName(footC, "Foot C");
  const char* d = fallbackFootName(footD, "Foot D");

  // Cantos
  _oled.setTextSize(1);
  _oled.setTextColor(SSD1306_WHITE);

  _oled.setCursor(0, 14);
  _oled.print(a);

  int16_t x1, y1;
  uint16_t w, h;

  _oled.getTextBounds(b, 0, 0, &x1, &y1, &w, &h);
  _oled.setCursor((int)kWidth - (int)w, 14);
  _oled.print(b);

  _oled.setCursor(0, 54);
  _oled.print(c);

  _oled.getTextBounds(d, 0, 0, &x1, &y1, &w, &h);
  _oled.setCursor((int)kWidth - (int)w, 54);
  _oled.print(d);

  // Centro vazio por padrão. Só mostra BPM se houver Tap com BPM válido.
  if (tapActive && bpmOr0 > 0) {
    char bpmNum[8];
    snprintf(bpmNum, sizeof(bpmNum), "%u", (unsigned)bpmOr0);

    // Número grande + "BPM" depois do número.
    _oled.setTextSize(3);
    _oled.getTextBounds(bpmNum, 0, 0, &x1, &y1, &w, &h);
    int numW = (int)w;
    int numX = (kWidth - numW) / 2;
    if (numX < 0) numX = 0;

    // Sobe um pouco o número para centralizar melhor.
    const int numY = 28;
    _oled.setCursor(numX, numY);
    _oled.print(bpmNum);

    _oled.setTextSize(1);
    const char* lbl = "BPM";
    _oled.getTextBounds(lbl, 0, 0, &x1, &y1, &w, &h);
    const int gap = 2;
    int lblX = numX + numW + gap;
    int lblY = numY + 10;  // alinha visualmente com a altura do texto grande
    if (lblX + (int)w <= (int)kWidth) {
      _oled.setCursor(lblX, lblY);
      _oled.print(lbl);
    }
  }

  _oled.setTextSize(1);
  _flush();
}

void Display::showMessage(const char* line1, const char* line2, const char* line3, const char* line4) {
  _header();
  int y = 16;
  const char* lines[4] = { line1, line2, line3, line4 };
  for (int i = 0; i < 4; i++) {
    if (!lines[i] || !lines[i][0]) continue;
    _oled.setCursor(0, y);
    _oled.print(lines[i]);
    y += 12;
  }
  _flush();
}

