"""Comunicação BLE GATT com o ESP32 (dispositivo polimidi)."""

from __future__ import annotations

import asyncio
import json
import sys
import threading
from typing import Any, Callable

from bleak import BleakClient, BleakScanner

DEVICE_HINT = "polimidi"
SERVICE_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a04"
CHAR_CMD_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a05"
CHAR_RSP_UUID = "7a5e9c10-b4d2-4e8f-9a1c-3d6f8e2b1a06"
REQUEST_TIMEOUT = 20.0
PING_TIMEOUT = 5.0
PING_INTERVAL_SEC = 3.0

_ERR_MAP: dict[str, str] = {
    "set_active falhou": "Não foi possível alterar o preset ativo no controlador.",
    "save falhou": "Não foi possível salvar o preset no controlador.",
    "save_settings falhou": "Não foi possível salvar as configurações no controlador.",
    "Não conectado.": "Não há conexão com o controlador.",
    "Falha ao conectar via BLE.": "Não foi possível conectar via Bluetooth.",
    "Sem resposta do dispositivo (timeout).": "O controlador não respondeu a tempo.",
    "Resposta BLE vazia.": "Resposta inválida do controlador.",
}


def friendly_error(message: str) -> str:
    """Traduz erros técnicos para mensagens ao usuário."""
    if message in _ERR_MAP:
        return _ERR_MAP[message]
    if message.startswith("Resposta inválida:"):
        return "Resposta inválida do controlador."
    if message.startswith("Brilho"):
        return message
    return message


def _configure_asyncio_for_ble() -> None:
    # bleak no Windows precisa de SelectorEventLoop (não Proactor/GUI).
    if sys.platform == "win32":
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())


_configure_asyncio_for_ble()


class PolimidiError(Exception):
    pass


class _AsyncWorker:
    """Um único loop asyncio em thread dedicada (compatível com tkinter no Windows)."""

    _instance: _AsyncWorker | None = None
    _lock = threading.Lock()

    def __init__(self) -> None:
        self._loop: asyncio.AbstractEventLoop | None = None
        self._ready = threading.Event()
        self._thread = threading.Thread(target=self._thread_main, name="polimidi-async", daemon=True)
        self._thread.start()
        if not self._ready.wait(timeout=10.0):
            raise PolimidiError("Falha ao iniciar loop asyncio para BLE.")

    @classmethod
    def instance(cls) -> _AsyncWorker:
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = cls()
        return cls._instance

    def _thread_main(self) -> None:
        _configure_asyncio_for_ble()
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        self._loop = loop
        self._ready.set()
        loop.run_forever()

    def run(self, coro: Any, timeout: float = REQUEST_TIMEOUT + 5) -> Any:
        if self._loop is None:
            raise PolimidiError("Loop asyncio indisponível.")
        fut = asyncio.run_coroutine_threadsafe(coro, self._loop)
        return fut.result(timeout=timeout)


class _BleSession:
    def __init__(self, address: str) -> None:
        self.address = address
        self._client: BleakClient | None = None
        self._rx_buf = bytearray()
        self._rx_event = asyncio.Event()
        self._last_line: bytes | None = None
        self._on_disconnect: Callable[[], None] | None = None

    def _on_notify(self, _sender: int, data: bytearray) -> None:
        self._rx_buf.extend(data)
        if b"\n" in self._rx_buf:
            line, rest = self._rx_buf.split(b"\n", 1)
            self._last_line = line
            self._rx_buf = bytearray(rest)
            self._rx_event.set()

    def _ble_disconnected(self, _client: BleakClient) -> None:
        if self._on_disconnect:
            self._on_disconnect()

    async def connect(self, on_disconnect: Callable[[], None] | None = None) -> None:
        self._on_disconnect = on_disconnect
        self._client = BleakClient(self.address, disconnected_callback=self._ble_disconnected)
        await self._client.connect()
        if not self._client.is_connected:
            raise PolimidiError("Falha ao conectar via BLE.")
        await self._client.start_notify(CHAR_RSP_UUID, self._on_notify)

    async def disconnect(self) -> None:
        if self._client and self._client.is_connected:
            try:
                await self._client.stop_notify(CHAR_RSP_UUID)
            except Exception:
                pass
            await self._client.disconnect()
        self._client = None
        self._on_disconnect = None

    def is_connected(self) -> bool:
        return self._client is not None and self._client.is_connected

    async def request(self, payload: dict[str, Any], timeout: float = REQUEST_TIMEOUT) -> Any:
        if not self._client or not self._client.is_connected:
            raise PolimidiError("Não conectado.")

        self._last_line = None
        self._rx_event.clear()

        line = json.dumps(payload, separators=(",", ":")) + "\n"
        data = line.encode("utf-8")
        chunk_max = 512
        for offset in range(0, len(data), chunk_max):
            await self._client.write_gatt_char(
                CHAR_CMD_UUID, data[offset : offset + chunk_max], response=False
            )

        try:
            await asyncio.wait_for(self._rx_event.wait(), timeout=timeout)
        except asyncio.TimeoutError as exc:
            raise PolimidiError("Sem resposta do dispositivo (timeout).") from exc

        if self._last_line is None:
            raise PolimidiError("Resposta BLE vazia.")

        try:
            return json.loads(self._last_line.decode("utf-8", errors="replace"))
        except json.JSONDecodeError as exc:
            raise PolimidiError(f"Resposta inválida: {self._last_line!r}") from exc


async def _scan_async(timeout: float = 6.0) -> list[tuple[str, str]]:
    found: list[tuple[str, str]] = []
    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        name = d.name or ""
        if DEVICE_HINT.lower() in name.lower():
            found.append((d.address, name))
    return found


class PolimidiLink:
    """API síncrona para uso com PyQt."""

    def __init__(self, address: str, on_disconnect: Callable[[], None] | None = None) -> None:
        self.address = address
        self._worker = _AsyncWorker.instance()
        self._session = _BleSession(address)
        self._lost = threading.Event()
        self._external_on_disconnect = on_disconnect
        self._worker.run(self._session.connect(on_disconnect=self._handle_ble_disconnect))

    def _handle_ble_disconnect(self) -> None:
        self._mark_lost()
        if self._external_on_disconnect:
            self._external_on_disconnect()

    def _mark_lost(self) -> None:
        self._lost.set()

    def is_lost(self) -> bool:
        return self._lost.is_set()

    def is_connected(self) -> bool:
        return not self._lost.is_set() and self._session.is_connected()

    def close(self) -> None:
        try:
            self._worker.run(self._session.disconnect())
        except Exception:
            pass
        self._mark_lost()

    @staticmethod
    def scan_devices(timeout: float = 6.0) -> list[tuple[str, str]]:
        return _AsyncWorker.instance().run(_scan_async(timeout), timeout=timeout + 5)

    def _request(self, payload: dict[str, Any], timeout: float = REQUEST_TIMEOUT) -> Any:
        if self.is_lost():
            raise PolimidiError("Não conectado.")
        return self._worker.run(
            self._session.request(payload, timeout=timeout),
            timeout=timeout + 5,
        )

    def ping(self) -> bool:
        """Keepalive: confirma que o dispositivo responde."""
        if self.is_lost():
            return False
        if not self._session.is_connected():
            self._mark_lost()
            return False
        try:
            data = self._worker.run(
                self._session.request({"op": "ping"}, timeout=PING_TIMEOUT),
                timeout=PING_TIMEOUT + 5,
            )
            ok = bool(data.get("ok")) and bool(data.get("connected", True))
            if not ok:
                self._mark_lost()
            return ok
        except PolimidiError:
            self._mark_lost()
            return False

    def get_active(self) -> int:
        data = self._request({"op": "get_active"})
        active = int(data.get("active", 1))
        if active < 1 or active > 10:
            return 1
        return active

    def get_presets(self) -> dict[int, dict | None]:
        data = self._request({"op": "get_presets"})
        out: dict[int, dict | None] = {}
        for k, v in data.items():
            try:
                idx = int(k)
            except (TypeError, ValueError):
                continue
            if 1 <= idx <= 10:
                out[idx] = v if isinstance(v, dict) else None
        return out

    def set_active(self, preset: int) -> None:
        resp = self._request({"op": "set_active", "active": preset})
        if not resp.get("ok"):
            raise PolimidiError(resp.get("err", "set_active falhou"))

    def save_preset(self, preset: int, data: dict) -> None:
        resp = self._request({"op": "save", "preset": preset, "data": data})
        if not resp.get("ok"):
            raise PolimidiError(resp.get("err", "save falhou"))

    def get_settings(self) -> dict[str, Any]:
        data = self._request({"op": "get_settings"})
        led = int(data.get("ledBrightness", 80))
        if led < 0:
            led = 0
        if led > 100:
            led = 100
        return {
            "ledBrightness": led,
            "midiClock": bool(data.get("midiClock", False)),
        }

    def save_settings(self, led_brightness: int, midi_clock: bool) -> None:
        if led_brightness < 0 or led_brightness > 100:
            raise PolimidiError("Brilho deve estar entre 0 e 100.")
        resp = self._request(
            {
                "op": "save_settings",
                "ledBrightness": led_brightness,
                "midiClock": midi_clock,
            }
        )
        if not resp.get("ok"):
            raise PolimidiError(resp.get("err", "save_settings falhou"))
