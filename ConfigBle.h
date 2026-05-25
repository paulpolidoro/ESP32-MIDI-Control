#ifndef CONFIG_BLE_H
#define CONFIG_BLE_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * Configuração via BLE GATT (peripheral), nome padrão "polimidi".
 * Protocolo: JSON terminado em \\n (write → notify, pode ser fragmentado).
 */
class ConfigBle {
public:
  static constexpr const char* DEFAULT_NAME = "polimidi";

  static constexpr const char* SERVICE_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a04";
  static constexpr const char* CHAR_CMD_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a05";
  static constexpr const char* CHAR_RSP_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a06";

  bool begin(const char* deviceName = DEFAULT_NAME);
  void update(Preferences& prefs);

  bool isConnected() const { return _connected; }

private:
  bool _connected;
  String _lineBuf;
  String _pendingLine;
  bool _hasPending;

  void _handleLine(Preferences& prefs, const String& line);
  void _reply(const char* json);

  friend class ConfigBleServerCallbacks;
  friend class ConfigBleCmdCallbacks;
};

#endif
