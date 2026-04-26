# Controlador MIDI (ESP32) — 4 Footswitches + BLE MIDI + Web Presets (Wi‑Fi)

Projeto de **controlador MIDI para ESP32** com:

- **4 footswitches** (cada um com **LED dedicado**)
- **MIDI via BLE** (Bluetooth Low Energy)
- **Display OLED SSD1306 via I2C**
- **Configuração de presets via Wi‑Fi** (página web) — opcional
- **Tap Tempo** por footswitch (modo configurável por preset)

## Hardware (pinos usados)

### Footswitches e LEDs
Cada footswitch usa 2 pinos: **{botão, LED}**.

| Foot | Botão (GPIO) | LED (GPIO) |
|------|--------------|------------|
| 1    | 18           | 19         |
| 2    | 4            | 5          |
| 3    | 12           | 13         |
| 4    | 22           | 23         |

> Fonte: `FOOT_PINS` em `controlador_midi.ino`.

### Display OLED (SSD1306 I2C)

- **I2C SDA**: GPIO **21**
- **I2C SCL**: GPIO **27**
- **Endereço I2C**: `0x3C`

> Observação: o projeto usa **SCL em 27** porque o GPIO 22 já está ocupado (footswitch 4).

## BLE MIDI

- **Nome do dispositivo BLE**: `MidiPortA`
- **Service UUID**: `03b80e5a-ede8-4b33-a751-6ce34ec4c700`
- **Characteristic UUID**: `7772e5db-3868-4112-a1a9-f2669d106bf3`

### Tap Tempo (CCs enviados)
Quando um foot está em modo **Tap Tempo**, o BPM calculado (faixa **40–300**) é enviado via BLE como:

- **CC 74**: “byte alto” do BPM (0, 1 ou 2)
- **CC 75**: “byte baixo” do BPM

Mapeamento:

- 40..127  → CC74 = 0, CC75 = 40..127  
- 128..255 → CC74 = 1, CC75 = 0..127  
- 256..300 → CC74 = 2, CC75 = 0..44  

Canal fixo usado pelo código: **canal 1** (no protocolo MIDI isso vira `0` internamente).

## Wi‑Fi + página web (opcional)

Quando habilitado, o ESP32 tenta conectar em uma rede Wi‑Fi e sobe um servidor HTTP na porta **80** com endpoints para:

- listar presets (`/presets`)
- ler preset ativo (`/active`)
- trocar preset ativo (`/setActive`, POST)
- salvar preset (`/save`, POST)
- página inicial (`/`)

### Arquivo de segredos (não versionar)
O projeto inclui `wifi_secrets.h` para SSID/senha e rede (IP fixo). **Não suba esse arquivo para o GitHub**.

Se ele não existir no seu ambiente, crie um `wifi_secrets.h` seguindo a ideia abaixo (exemplo):

```cpp
// wifi_secrets.h (exemplo)
#pragma once
#include <WiFi.h>

#define WIFI_SSID "SUA_REDE"
#define WIFI_PASS "SUA_SENHA"

static const IPAddress WIFI_LOCAL_IP(192,168,1,70);
static const IPAddress WIFI_GATEWAY(192,168,1,1);
static const IPAddress WIFI_SUBNET(255,255,255,0);
static const IPAddress WIFI_DNS_PRIMARY(1,1,1,1);
static const IPAddress WIFI_DNS_SECONDARY(8,8,8,8);
```

Sugestão: adicione ao seu `.gitignore`:

```gitignore
wifi_secrets.h
```

## Compilação / Upload (Arduino IDE)

### Memória flash (partição)
O sketch pode ficar grande por causa do Wi‑Fi/Web. Se aparecer erro de “flash cheia”, use:

- **Ferramentas → Esquema de partições → “Huge APP (3MB No OTA/1MB SPIFFS)”**

Se preferir compilar **sem Wi‑Fi/página web**, desabilite a flag:

- No topo de `controlador_midi.ino`:
  - `#define ENABLE_WIFI_WEB 0`

### Placa
O código é para **ESP32** (Arduino core ESP32). Selecione a placa equivalente ao seu módulo (ex.: DevKit).

## Estrutura (arquivos principais)

- `controlador_midi.ino`: setup/loop, pinos, display, BLE, web server (opcional)
- `MidiPresetRunner.*`: lógica de presets, execução de comandos, Tap Tempo e persistência (NVS/Preferences)
- `TapTempo.*`: cálculo do BPM por taps e “batida” para piscar LED

## Licença
Defina a licença do repositório conforme sua preferência (MIT, GPL, etc.).

