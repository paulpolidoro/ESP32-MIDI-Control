#include "ConfigBle.h"
#include "MidiPresetRunner.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

static ConfigBle* s_instance = nullptr;
static BLECharacteristic* s_rspChar = nullptr;

class ConfigBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    (void)pServer;
    if (s_instance) {
      s_instance->_connected = true;
    }
  }

  void onDisconnect(BLEServer* pServer) override {
    (void)pServer;
    if (s_instance) {
      s_instance->_connected = false;
      s_instance->_lineBuf = "";
      s_instance->_pendingLine = "";
      s_instance->_hasPending = false;
    }
    BLEDevice::startAdvertising();
  }
};

class ConfigBleCmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    if (!s_instance) return;
    String value = pChar->getValue();
    if (value.length() == 0) return;

    for (size_t i = 0; i < value.length(); i++) {
      char c = value.charAt(i);
      if (c == '\r') continue;
      if (c == '\n') {
        if (s_instance->_lineBuf.length() > 0) {
          s_instance->_pendingLine = s_instance->_lineBuf;
          s_instance->_lineBuf = "";
          s_instance->_hasPending = true;
        }
        continue;
      }
      if (s_instance->_lineBuf.length() < 4096) {
        s_instance->_lineBuf += c;
      }
    }
  }
};

bool ConfigBle::begin(const char* deviceName) {
  s_instance = this;
  _connected = false;
  _lineBuf.reserve(128);
  _hasPending = false;

  BLEDevice::init(deviceName);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ConfigBleServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);
  BLECharacteristic* cmdChar = service->createCharacteristic(
    CHAR_CMD_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  cmdChar->setCallbacks(new ConfigBleCmdCallbacks());

  s_rspChar = service->createCharacteristic(
    CHAR_RSP_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  s_rspChar->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  return true;
}

void ConfigBle::_reply(const char* json) {
  if (!_connected || !s_rspChar || !json) return;

  String payload = json;
  payload += '\n';
  const size_t chunkMax = 512;
  size_t offset = 0;
  const size_t total = payload.length();

  while (offset < total) {
    size_t n = total - offset;
    if (n > chunkMax) n = chunkMax;
    s_rspChar->setValue((uint8_t*)(payload.c_str() + offset), n);
    s_rspChar->notify();
    offset += n;
    delay(2);
  }
}

void ConfigBle::update(Preferences& prefs) {
  if (_hasPending) {
    String line = _pendingLine;
    _pendingLine = "";
    _hasPending = false;
    _handleLine(prefs, line);
  }
}

void ConfigBle::_handleLine(Preferences& prefs, const String& line) {
  StaticJsonDocument<128> reqHead;
  DeserializationError err = deserializeJson(reqHead, line.c_str());
  if (err) {
    _reply("{\"ok\":false,\"err\":\"json\"}");
    return;
  }

  const char* op = reqHead["op"] | "";
  if (!strcmp(op, "ping")) {
    _reply("{\"ok\":true,\"connected\":true}");
    return;
  }

  if (!strcmp(op, "get_active")) {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"active\":%d}", MidiPresetRunner::getActivePreset());
    _reply(buf);
    return;
  }

  if (!strcmp(op, "get_presets")) {
    String out = "{";
    for (int i = 1; i <= 10; i++) {
      char key[6];
      snprintf(key, sizeof(key), "p%d", i);
      String s = prefs.getString(key, "");
      if (i > 1) out += ',';
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
    _reply(out.c_str());
    return;
  }

  if (!strcmp(op, "set_active")) {
    int n = reqHead["active"] | 0;
    if (n < 1 || n > 10) {
      _reply("{\"ok\":false,\"err\":\"range\"}");
      return;
    }
    MidiPresetRunner::setActivePreset(n);
    _reply("{\"ok\":true}");
    return;
  }

  if (!strcmp(op, "save")) {
    StaticJsonDocument<8192> req;
    err = deserializeJson(req, line.c_str());
    if (err) {
      _reply("{\"ok\":false,\"err\":\"json\"}");
      return;
    }
    int n = req["preset"] | 0;
    if (n < 1 || n > 10) {
      _reply("{\"ok\":false,\"err\":\"preset\"}");
      return;
    }
    if (req["data"].isNull()) {
      _reply("{\"ok\":false,\"err\":\"data\"}");
      return;
    }
    String j;
    serializeJson(req["data"], j);
    if (j.length() > 3800) {
      _reply("{\"ok\":false,\"err\":\"size\"}");
      return;
    }
    char key[6];
    snprintf(key, sizeof(key), "p%d", n);
    size_t written = prefs.putString(key, j);
    if (written != j.length()) {
      _reply("{\"ok\":false,\"err\":\"nvs\"}");
      return;
    }
    MidiPresetRunner::notifyPresetSlotSaved(n);
    _reply("{\"ok\":true}");
    return;
  }

  if (!strcmp(op, "get_settings")) {
    char buf[80];
    snprintf(buf, sizeof(buf),
             "{\"ledBrightness\":%u,\"midiClock\":%s}",
             (unsigned)MidiPresetRunner::getLedBrightness(),
             MidiPresetRunner::isMidiClockEnabled() ? "true" : "false");
    _reply(buf);
    return;
  }

  if (!strcmp(op, "save_settings")) {
    int led = reqHead["ledBrightness"] | -1;
    if (led < 0 || led > 100) {
      _reply("{\"ok\":false,\"err\":\"brightness\"}");
      return;
    }
    bool clk = reqHead["midiClock"] | false;
    MidiPresetRunner::saveDeviceSettings((uint8_t)led, clk);
    _reply("{\"ok\":true}");
    return;
  }

  _reply("{\"ok\":false,\"err\":\"op\"}");
}
