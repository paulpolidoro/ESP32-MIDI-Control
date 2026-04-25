// Controlador MIDI - 4 footswitches, BLE e configuração via WiFi (página web)
//
// FLASH CHEIA? O IDE limita o app à partição padrão (~1,3 MB). Aumente:
//   Ferramentas → Esquema de partições → "Huge APP (3MB No OTA/1MB SPIFFS)"
//   (ou "16MB Flash (3MB APP...)" se sua placa tiver 16 MB). Placa com 4 MB
//   de flash costuma aceitar Huge APP.
// Opcional: Ferramentas → Core Debug Level → Nenhum (economiza um pouco).
//
// Sem mudar partição, defina ENABLE_WIFI_WEB como 0 para compilar só BLE+foots
// (sem WiFi nem página web).
#ifndef ENABLE_WIFI_WEB
#define ENABLE_WIFI_WEB 1
#endif

#include "Foot.h"
#include "Bluetooth.h"
#include "MidiPresetRunner.h"
#include "Display.h"
#include <Preferences.h>
#if ENABLE_WIFI_WEB
#include <WiFi.h>
#include <WebServer.h>
#include "wifi_secrets.h"
#include "welcome_page.h"
#endif

Preferences gMidiPrefs;

// OLED SSD1306 I2C (ESP32 30 pinos): SDA=21 é comum; SCL aqui usa 27 porque 22 está ocupado no projeto.
static Display display(0x3C, 21, 27);
static bool displayOk = false;

static void footPresetPress(int footId, Foot* foot) {
  MidiPresetRunner::handleFootPress(footId, foot);
}

const int NUM_FEET = 4;
const int FOOT_PINS[NUM_FEET][2] = { {18, 19}, {4, 5}, {12, 13}, {22, 23} };  // {botão, LED}

Foot foot1(0, FOOT_PINS[0][0], FOOT_PINS[0][1]);
Foot foot2(1, FOOT_PINS[1][0], FOOT_PINS[1][1]);
Foot foot3(2, FOOT_PINS[2][0], FOOT_PINS[2][1]);
Foot foot4(3, FOOT_PINS[3][0], FOOT_PINS[3][1]);
Foot* const feet[NUM_FEET] = { &foot1, &foot2, &foot3, &foot4 };

const char* BLE_DEVICE_NAME = "MidiPortA";
const char* BLE_SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
const char* BLE_CHARACTERISTIC_UUID = "7772e5db-3868-4112-a1a9-f2669d106bf3";

uint8_t _lastCcNumber = 0;
uint8_t _lastValue = 0;

Bluetooth ble(BLE_DEVICE_NAME, BLE_SERVICE_UUID, BLE_CHARACTERISTIC_UUID);

#if ENABLE_WIFI_WEB
WebServer webServer(80);

static void handleActiveGet() {
  char buf[28];
  snprintf(buf, sizeof(buf), "{\"active\":%d}", MidiPresetRunner::getActivePreset());
  webServer.send(200, "application/json; charset=utf-8", buf);
}

static void handlePresetsList() {
  String out = "{";
  for (int i = 1; i <= 10; i++) {
    char key[6];
    snprintf(key, sizeof(key), "p%d", i);
    String s = gMidiPrefs.getString(key, "");
    if (i > 1) {
      out += ',';
    }
    out += '"';
    out += String(i);
    out += "\":";
    if (s.length() == 0) {
      out += "null";
    } else {
      out += s;
    }
  }
  out += "}";
  webServer.send(200, "application/json; charset=utf-8", out);
}

static void handlePresetSave() {
  if (!webServer.hasArg("preset") || !webServer.hasArg("json")) {
    webServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"missing\"}");
    return;
  }
  int n = webServer.arg("preset").toInt();
  if (n < 1 || n > 10) {
    webServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"preset\"}");
    return;
  }
  String j = webServer.arg("json");
  if (j.length() > 3800) {
    webServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"size\"}");
    return;
  }
  char key[6];
  snprintf(key, sizeof(key), "p%d", n);
  size_t written = gMidiPrefs.putString(key, j);
  if (written != j.length()) {
    webServer.send(500, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"nvs\"}");
    return;
  }
  MidiPresetRunner::notifyPresetSlotSaved(n);
  webServer.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

static void handleSetActivePost() {
  if (!webServer.hasArg("active")) {
    webServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"missing\"}");
    return;
  }
  int n = webServer.arg("active").toInt();
  if (n < 1 || n > 10) {
    webServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"err\":\"range\"}");
    return;
  }
  MidiPresetRunner::setActivePreset(n);
  webServer.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

static bool connectWifiStatic() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (!WiFi.config(WIFI_LOCAL_IP, WIFI_GATEWAY, WIFI_SUBNET, WIFI_DNS_PRIMARY, WIFI_DNS_SECONDARY)) {
    Serial.println("[WiFi] WiFi.config() falhou (IP fixo).");
    return false;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  const unsigned long timeoutMs = 25000;
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

static void handleWebRoot() {
  webServer.send_P(200, "text/html; charset=utf-8", WELCOME_HTML);
}
#endif

void onBleReceive(const uint8_t* d, size_t length) {
  MidiPresetRunner::scanIncomingBle(d, length);

  if (length >= 5 && d[3] == _lastCcNumber && d[4] == _lastValue) {
    _lastCcNumber = 0;
    _lastValue = 0;
    Serial.println("Ignoring duplicate packet");
  }
}

void sendCCToBle(uint8_t channel, uint8_t ccNumber, uint8_t value) {
  _lastCcNumber = ccNumber;
  _lastValue = value;
  ble.sendCC(channel, ccNumber, value);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  displayOk = display.begin();
  if (displayOk) {
    display.showBoot("Controlador MIDI");
    display.setBleState(Display::BleState::Connecting);
    display.setWifi(false, IPAddress(0, 0, 0, 0));
  }

  gMidiPrefs.begin("midi_pr", false);

#if ENABLE_WIFI_WEB
  if (connectWifiStatic()) {
    Serial.print("[WiFi] Conectado. IP: ");
    Serial.println(WiFi.localIP());
    if (displayOk) display.setWifi(true, WiFi.localIP());
    webServer.on("/", HTTP_GET, handleWebRoot);
    webServer.on("/presets", HTTP_GET, handlePresetsList);
    webServer.on("/active", HTTP_GET, handleActiveGet);
    webServer.on("/setActive", HTTP_POST, handleSetActivePost);
    webServer.on("/save", HTTP_POST, handlePresetSave);
    webServer.begin();
    Serial.println("[Web] Servidor em http://192.168.1.70/ (ou o IP configurado)");
  } else {
    Serial.println("[WiFi] Sem conexão — footswitches e BLE seguem; web desligada.");
    if (displayOk) display.setWifi(false, IPAddress(0, 0, 0, 0));
  }
#endif

  MidiPresetRunner::begin(gMidiPrefs, &ble);

  for (int i = 0; i < NUM_FEET; i++) {
    feet[i]->begin();
    feet[i]->setOnFootPress(footPresetPress);
  }
  ble.begin();
  ble.setOnReceive(onBleReceive);

  if (displayOk) {
    display.showMessage("Conectando...", nullptr, nullptr, nullptr);
  }
}

void loop() {
#if ENABLE_WIFI_WEB
  if (WiFi.status() == WL_CONNECTED) {
    webServer.handleClient();
  }
#endif

  for (int i = 0; i < NUM_FEET; i++) feet[i]->update();
  ble.update();
  if (displayOk) display.update();

  static bool wasConnected = false;
  bool connected = ble.isConnected();

  if (connected && !wasConnected) {
    if (displayOk) {
      display.setBleState(Display::BleState::Connected);
      display.showMessage("Conectado", nullptr, nullptr, nullptr);
    }
  } else if (!connected && wasConnected) {
    if (displayOk) {
      display.setBleState(Display::BleState::Connecting);
      display.showMessage("Conectando...", nullptr, nullptr, nullptr);
    }
  } else if (!connected && !wasConnected) {
    if (displayOk) display.setBleState(Display::BleState::Connecting);
  }
  wasConnected = connected;
}
