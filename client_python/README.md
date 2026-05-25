# Cliente Python — Polimidi

Configurador do controlador MIDI **Polimidi** via **Bluetooth**, com interface **PyQt6** (tema escuro).

## Pré-requisitos

1. Bluetooth ligado no computador (Windows 10/11).
2. Controlador com firmware atualizado.
3. Python 3.10 ou superior.

## Instalação

```bash
cd client_python
pip install -r requirements.txt
```

## Uso

```bash
python polimidi_config.py
```

1. **Buscar** — localiza controladores Polimidi por perto.
2. **Conectar** — carrega presets, preset ativo e configurações.
3. Edite os pedais A–D e clique em **Salvar preset**.
4. Aba **Configurações** — brilho dos LEDs e relógio MIDI; clique em **Salvar configurações**.

Enquanto conectado, o app monitora a conexão e avisa se o controlador desconectar.

## Protocolo (desenvolvedores)
| UUID | Função |
|------|--------|
| `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a04` | Serviço |
| `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a05` | Comando (write) |
| `7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a06` | Resposta (notify) |

Protocolo: JSON + `\n` (pode ser fragmentado em vários pacotes BLE).

Comandos: `get_active`, `get_presets`, `set_active`, `save`, `get_settings`, `save_settings`, `ping`.

`ping`: keepalive — resposta `{"ok":true,"connected":true}` (app envia a cada ~3 s enquanto conectado).

`save_settings`: `{"op":"save_settings","ledBrightness":80,"midiClock":true}`

Resposta `get_settings`: `{"ledBrightness":80,"midiClock":false}`
