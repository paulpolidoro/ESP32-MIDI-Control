#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stddef.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteCharacteristic.h>

/**
 * Callback chamado quando dados são recebidos via notify.
 * pData = ponteiro para os bytes, length = quantidade.
 */
typedef void (*OnNotifyCallback)(const uint8_t* pData, size_t length);

/**
 * Classe Bluetooth - cliente BLE.
 * Conecta a um dispositivo pelo nome, usa Service UUID e Characteristic UUID
 * para assinar notify e enviar dados. Tenta reconectar a cada 5 segundos
 * e imprime o status no Serial.
 */
class Bluetooth {
public:
  /**
   * Construtor.
   * @param deviceName        Nome do dispositivo BLE (advertised name)
   * @param serviceUUID       UUID do serviço (ex.: "03b80e5a-ede8-4b33-a751-6ce34ec4c700")
   * @param characteristicUUID UUID da característica (Notify + Write)
   */
  Bluetooth(const char* deviceName, const char* serviceUUID, const char* characteristicUUID);

  /**
   * Inicializa o BLE. Chamar uma vez no setup().
   */
  void begin();

  /**
   * Deve ser chamado no loop(). Verifica conexão; se desconectado, tenta reconectar a cada 5 s e imprime status.
   */
  void update();

  /**
   * Retorna true se estiver conectado.
   */
  bool isConnected() const { return _connected; }

  /**
   * Envia dados para a característica (write).
   */
  void write(const uint8_t* data, size_t length);

  /**
   * Envia um comando MIDI Control Change (CC) em formato BLE MIDI.
   * @param channel  Canal MIDI (0 a 15; canal 1 = 0)
   * @param ccNumber Número do controlador (0 a 127)
   * @param value    Valor (0 a 127)
   */
  void sendCC(uint8_t channel, uint8_t ccNumber, uint8_t value);

  /** Program Change: canal 0–15, programa 0–127 */
  void sendPC(uint8_t channel, uint8_t program);

  void setOnReceive(OnNotifyCallback callback) { _onNotify = callback; }
  void setOnNotify(OnNotifyCallback callback) { _onNotify = callback; }

  /**
   * Desconecta do dispositivo.
   */
  void disconnect();

private:
  String _deviceName;
  String _serviceUUID;
  String _characteristicUUID;
  bool _connected;
  OnNotifyCallback _onNotify;
  unsigned long _lastAttemptMs;
  static const unsigned long RECONNECT_INTERVAL_MS = 5000;

  BLEClient* _pClient;
  BLERemoteCharacteristic* _pChar;

  bool _doConnect();
  void _printStatus(const char* msg);
  void _buildMidiTimestamp(uint8_t* header, uint8_t* tsLow);
  static void _notifyCallbackStatic(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);
  static Bluetooth* _instanceForNotify;
};

#endif // BLUETOOTH_H
