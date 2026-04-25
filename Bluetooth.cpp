#include "Bluetooth.h"

Bluetooth* Bluetooth::_instanceForNotify = nullptr;

Bluetooth::Bluetooth(const char* deviceName, const char* serviceUUID, const char* characteristicUUID)
  : _deviceName(deviceName),
    _serviceUUID(serviceUUID),
    _characteristicUUID(characteristicUUID),
    _connected(false),
    _onNotify(nullptr),
    _lastAttemptMs(0),
    _pClient(nullptr),
    _pChar(nullptr) {
}

void Bluetooth::begin() {
  BLEDevice::init("ControladorMIDI");
  _pClient = BLEDevice::createClient();
  _lastAttemptMs = millis() - RECONNECT_INTERVAL_MS;
  _printStatus("BLE iniciado.");
}

void Bluetooth::_printStatus(const char* msg) {
  Serial.print("[BLE] ");
  Serial.println(msg);
}

void Bluetooth::update() {
  _connected = (_pClient && _pClient->isConnected());
  if (!_connected) _pChar = nullptr;

  if (_connected) return;
  if (millis() - _lastAttemptMs < RECONNECT_INTERVAL_MS) return;

  _lastAttemptMs = millis();
  _printStatus("Tentando conectar...");
  _printStatus(_doConnect() ? "Conectado." : "Falha. Proxima tentativa em 5s.");
}

bool Bluetooth::_doConnect() {
  if (_pClient && _pClient->isConnected()) {
    _pClient->disconnect();
  }
  _pChar = nullptr;

  _printStatus("Escaneando...");
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setActiveScan(true);
  BLEScanResults* results = pScan->start(5, false);  // 5 segundos de scan; retorna ponteiro
  if (!results) {
    _printStatus("Scan falhou.");
    return false;
  }

  int targetIndex = -1;
  for (int i = 0; i < results->getCount(); i++) {
    if (results->getDevice(i).getName() == _deviceName) {
      targetIndex = i;
      break;
    }
  }

  if (targetIndex < 0) {
    _printStatus("Dispositivo nao encontrado no scan.");
    pScan->clearResults();
    return false;
  }

  _printStatus("Dispositivo encontrado, conectando...");
  BLEAdvertisedDevice target = results->getDevice(targetIndex);
  pScan->clearResults();

  if (!_pClient->connect(&target)) {
    _printStatus("Erro ao conectar (connect falhou).");
    return false;
  }

  BLEUUID svcUUID(_serviceUUID.c_str());
  BLEUUID charUUID(_characteristicUUID.c_str());
  BLERemoteService* pService = _pClient->getService(svcUUID);
  if (!pService) {
    _printStatus("Servico nao encontrado.");
    _pClient->disconnect();
    return false;
  }

  _pChar = pService->getCharacteristic(charUUID);
  if (!_pChar) {
    _printStatus("Caracteristica nao encontrada.");
    _pClient->disconnect();
    return false;
  }

  _instanceForNotify = this;
  _pChar->registerForNotify(_notifyCallbackStatic);
  _connected = true;
  return true;
}

void Bluetooth::_notifyCallbackStatic(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  (void)pChar;
  (void)isNotify;
  if (_instanceForNotify && _instanceForNotify->_onNotify) {
    _instanceForNotify->_onNotify(pData, length);
  }
}

void Bluetooth::write(const uint8_t* data, size_t length) {
  if (!_connected || !_pChar) return;
  _pChar->writeValue((uint8_t*)data, length);
}

void Bluetooth::_buildMidiTimestamp(uint8_t* header, uint8_t* tsLow) {
  uint16_t ts = (uint16_t)(millis() & 0x1FFF);  // 13 bits
  *header = 0x80 | (uint8_t)((ts >> 7) & 0x3F);
  *tsLow  = 0x80 | (uint8_t)(ts & 0x7F);
}

void Bluetooth::sendCC(uint8_t channel, uint8_t ccNumber, uint8_t value) {
  if (!_connected || !_pChar) return;
  channel   = channel & 0x0F;
  ccNumber  = ccNumber & 0x7F;
  value     = value & 0x7F;
  uint8_t header, tsLow;
  _buildMidiTimestamp(&header, &tsLow);
  uint8_t packet[5] = {
    header,
    tsLow,
    (uint8_t)(0xB0 | channel),
    ccNumber,
    value
  };
  _pChar->writeValue(packet, sizeof(packet));
}

void Bluetooth::sendPC(uint8_t channel, uint8_t program) {
  if (!_connected || !_pChar) return;
  channel = channel & 0x0F;
  program = program & 0x7F;
  uint8_t header, tsLow;
  _buildMidiTimestamp(&header, &tsLow);
  uint8_t packet[4] = {
    header,
    tsLow,
    (uint8_t)(0xC0 | channel),
    program
  };
  _pChar->writeValue(packet, sizeof(packet));
}

void Bluetooth::disconnect() {
  if (_pClient && _pClient->isConnected()) {
    _pClient->disconnect();
  }
  _connected = false;
  _pChar = nullptr;
}
