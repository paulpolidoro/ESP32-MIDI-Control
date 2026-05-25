# Controlador MIDI (ESP32)

Controlador com **4 footswitches**, **MIDI por cabo DIN**, **display OLED** e **configuração via Bluetooth** (app Polimidi).

Cliente de configuração: [`client_python/`](client_python/README.md) (PyQt6).

---

## Mapa de pinos (ESP32 clássico)

| GPIO | Função | Observação |
|------|--------|------------|
| **4** | Foot 2 — botão | Entrada, pull-up interno |
| **5** | Foot 2 — LED | Saída |
| **12** | Foot 3 — botão | Entrada |
| **13** | Foot 3 — LED | Saída |
| **14** | **MIDI TX** (Serial2) | Saída UART → circuito MIDI OUT (DIN) |
| **18** | Foot 1 — botão | Entrada |
| **19** | Foot 1 — LED | Saída |
| **21** | OLED **SDA** (I2C) | Endereço `0x3C` |
| **22** | Foot 4 — botão | Entrada |
| **23** | Foot 4 — LED | Saída |
| **27** | OLED **SCL** (I2C) | SCL em 27 (22 ocupado pelo foot 4) |
| **34** | **MIDI RX** (Serial2) | **Só entrada**, máx. **3,3 V** — opto no MIDI IN |
| **1 / 3** | USB-Serial | Opcional (monitor serial, se habilitado na placa) |

**Não use GPIO 35 para TX** — pinos **34–39** são somente entrada no ESP32 clássico.

**BLE** (`polimidi`) usa rádio interno; não ocupa GPIO.

---

## MIDI (Serial2 @ 31250 baud)

| Direção | GPIO | Circuito |
|---------|------|----------|
| ESP → pedaleira (OUT) | **14** | GPIO 14 → transistor NPN → DIN pino 5; pino 4 → GND |
| Pedaleira → ESP (IN) | **34** | DIN pino 5 → opto (6N138/PC900) → GPIO 34 (3,3 V) |

Se o OUT não funcionar, em `HardwareMidi.h` teste:

```cpp
#define MIDI_UART_INVERT true
```

### Comandos por foot (preset)

Formato por linha: `canal-tipo-parâmetros`

- `1-PC-1` — Program Change, canal 1, programa 1  
- `1-CC-50-12` — Control Change 50, valor 12  

### Tap Tempo

Foot em modo tap envia no **canal 1**: **CC 74** + **CC 75** (BPM 40–300).

### MIDI Clock (sincronização com pedaleira)

Com **relógio MIDI** habilitado (aba Configurações no app), o dispositivo escuta mensagens no MIDI IN:

| Byte | Função |
|------|--------|
| `0xF8` | Clock (24 ticks = 1 semínima) |
| `0xFA` | Start |
| `0xFB` | Continue |
| `0xFC` | Stop |

O BPM é calculado a partir do intervalo entre semínimas. Com foot em **Tap Tempo**, o LED pulsa no beat e o BPM aparece no display. **CC 74/75 só são enviados no tap manual pelo foot**, não quando o tempo vem do clock externo.

### Preset via MIDI IN

**Program Change** com programa **1–10** (byte MIDI 1–10) troca o preset ativo.

---

## Configuração BLE GATT

| Item | Valor |
|------|--------|
| Nome | `polimidi` |
| Serviço | `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a04` |
| Comando (write) | `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a05` |
| Resposta (notify) | `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a06` |

10 presets independentes; cada foot (A–D) com listas A/B, LEDs e modo tap.

Comandos BLE adicionais: `get_settings` / `save_settings` (`ledBrightness` 0–100, `midiClock` bool).

---

## Compilação (Arduino IDE)

- Placa: **ESP32**
- Bibliotecas: **ArduinoJson**, **Adafruit SSD1306/GFX**
- Ative **Bluetooth** nas ferramentas da placa

---

## Estrutura do firmware

| Arquivo | Função |
|---------|--------|
| `controlador_midi.ino` | Setup, loop, pinos |
| `HardwareMidi.*` | UART MIDI (Serial2) |
| `ConfigBle.*` | Servidor BLE GATT |
| `MidiPresetRunner.*` | Presets, comandos, tap tempo, MIDI clock, NVS |
| `DeviceSettings.*` | Brilho LED e flag MIDI Clock (NVS) |
| `Foot.*` / `Led.*` | Footswitches e LEDs |
| `Display.*` | OLED SSD1306 |
| `TapTempo.*` | Cálculo de BPM |
| `client_python/` | Configurador PyQt6 |
