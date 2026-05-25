#!/usr/bin/env python3
"""Configurador Polimidi — controlador MIDI via Bluetooth (PyQt6)."""

from __future__ import annotations

import sys
import threading

from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QColor, QFont, QPalette
from PyQt6.QtWidgets import (
    QApplication,
    QButtonGroup,
    QCheckBox,
    QComboBox,
    QFrame,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QRadioButton,
    QScrollArea,
    QSlider,
    QStatusBar,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from polimidi_link import PING_INTERVAL_SEC, PolimidiLink, friendly_error

APP_TITLE = "Polimidi"

STYLESHEET = """
/* Base global — evita fundo branco / texto preto em widgets filhos */
QWidget {
    color: #e2e8f0;
    background-color: transparent;
}

QMainWindow, QWidget#centralRoot {
    background-color: #0f1419;
}

QWidget#footPanel {
    background-color: #1a2332;
}

QFrame#headerBar {
    background-color: #1a2332;
    border: 1px solid #334155;
    border-radius: 12px;
}

QFrame#helpBox {
    background-color: rgba(56, 189, 248, 0.08);
    border: 1px solid #334155;
    border-radius: 10px;
}

QFrame#footerBar {
    background-color: #1a2332;
    border: 1px solid #334155;
    border-radius: 12px;
}

QLabel {
    color: #e2e8f0;
    background: transparent;
}

QLabel#appTitle {
    font-size: 16pt;
    font-weight: 600;
    color: #f1f5f9;
}

QLabel#appSubtitle {
    color: #94a3b8;
    font-size: 9pt;
}

QLabel#statusPill {
    background-color: #334155;
    color: #cbd5e1;
    border-radius: 10px;
    padding: 4px 12px;
    font-size: 9pt;
}

QLabel#statusPill[connected="true"] {
    background-color: rgba(34, 197, 94, 0.2);
    color: #4ade80;
}

QLabel#statusBadge {
    background-color: rgba(56, 189, 248, 0.15);
    color: #38bdf8;
    border: 1px solid rgba(56, 189, 248, 0.35);
    border-radius: 8px;
    padding: 6px 14px;
    font-weight: 600;
}

QLabel#footTitle {
    font-size: 13pt;
    font-weight: 600;
    color: #f1f5f9;
}

QLabel#hintLabel {
    color: #94a3b8;
    font-size: 9pt;
}

QLabel#helpTitle {
    font-weight: 600;
    color: #e2e8f0;
}

QLabel#helpBody {
    color: #94a3b8;
}

QLabel#saveStatus {
    color: #94a3b8;
}

QComboBox, QLineEdit, QTextEdit {
    background-color: #0f1419;
    color: #e2e8f0;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 6px 10px;
    selection-background-color: #38bdf8;
    selection-color: #0f172a;
}

QLineEdit:disabled, QTextEdit:disabled, QComboBox:disabled {
    background-color: #1e293b;
    color: #64748b;
}

QComboBox::drop-down {
    border: none;
    width: 24px;
}

QComboBox QAbstractItemView {
    background-color: #1a2332;
    color: #e2e8f0;
    border: 1px solid #334155;
    selection-background-color: #38bdf8;
    selection-color: #0f172a;
}

QTabWidget {
    background-color: #0f1419;
}

QTabWidget::pane {
    border: 1px solid #334155;
    border-radius: 10px;
    background-color: #1a2332;
    top: -1px;
}

QTabBar {
    background: transparent;
}

QTabBar::tab {
    background: #0f1419;
    color: #94a3b8;
    padding: 10px 18px;
    margin-right: 4px;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    font-weight: 500;
}

QTabBar::tab:selected {
    background: #1a2332;
    color: #38bdf8;
    border: 1px solid #334155;
    border-bottom: none;
}

QTabBar::tab:hover:!selected {
    color: #e2e8f0;
    background: rgba(56, 189, 248, 0.08);
}

QGroupBox {
    background-color: #0f1419;
    border: 1px solid #334155;
    border-radius: 10px;
    margin-top: 14px;
    padding: 16px 12px 12px 12px;
    font-weight: 600;
    color: #94a3b8;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 6px;
    color: #94a3b8;
}

QRadioButton {
    spacing: 8px;
    color: #e2e8f0;
    background: transparent;
}

QRadioButton:disabled {
    color: #64748b;
}

QRadioButton::indicator {
    width: 16px;
    height: 16px;
    border-radius: 8px;
    border: 2px solid #475569;
    background: #0f1419;
}

QRadioButton::indicator:checked {
    border: 2px solid #38bdf8;
    background: #38bdf8;
}

QRadioButton::indicator:disabled {
    border-color: #334155;
    background: #1e293b;
}

QPushButton {
    background-color: #1e293b;
    color: #e2e8f0;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 500;
}

QPushButton:hover {
    background-color: #334155;
    border-color: #475569;
}

QPushButton:pressed {
    background-color: #0f172a;
}

QPushButton#primaryBtn {
    background-color: #38bdf8;
    color: #0f172a;
    border: none;
    font-weight: 600;
    padding: 10px 22px;
}

QPushButton#primaryBtn:hover {
    background-color: #7dd3fc;
}

QPushButton#primaryBtn:pressed {
    background-color: #0ea5e9;
}

QPushButton:disabled {
    color: #64748b;
    background-color: #1e293b;
}

QLabel#saveStatusOk {
    color: #4ade80;
}

QScrollArea#footScroll {
    background-color: #1a2332;
}

QScrollArea#footScroll QAbstractScrollArea::viewport {
    background-color: #1a2332;
}

QScrollArea > QWidget > QWidget {
    background-color: #1a2332;
}

QAbstractScrollArea::viewport {
    background-color: #1a2332;
}

QScrollBar:vertical {
    background: #0f1419;
    width: 10px;
    margin: 0;
    border-radius: 5px;
}

QScrollBar::handle:vertical {
    background: #334155;
    min-height: 24px;
    border-radius: 5px;
}

QScrollBar::handle:vertical:hover {
    background: #475569;
}

QScrollBar:horizontal {
    background: #0f1419;
    height: 10px;
    margin: 0;
    border-radius: 5px;
}

QScrollBar::handle:horizontal {
    background: #334155;
    min-width: 24px;
    border-radius: 5px;
}

QScrollBar::add-line, QScrollBar::sub-line {
    width: 0;
    height: 0;
}

QStatusBar {
    background: #0f1419;
    color: #64748b;
    border-top: 1px solid #1e293b;
}

QMessageBox {
    background-color: #1a2332;
}

QMessageBox QLabel {
    color: #e2e8f0;
}
"""


def empty_foot() -> dict:
    return {
        "name": "",
        "mode": "press",
        "press": "unique",
        "listA": "",
        "listB": "",
        "ledA": "off",
        "ledB": "off",
    }


def empty_preset() -> dict:
    return {"feet": [empty_foot() for _ in range(4)]}


class ScanWorker(QThread):
    finished_ok = pyqtSignal(list)
    finished_err = pyqtSignal(str)

    def run(self) -> None:
        try:
            self.finished_ok.emit(PolimidiLink.scan_devices())
        except Exception as exc:
            self.finished_err.emit(str(exc))


class ConnectWorker(QThread):
    finished_ok = pyqtSignal(object, int, dict, dict)
    finished_err = pyqtSignal(str)

    def __init__(self, address: str, old_link: PolimidiLink | None) -> None:
        super().__init__()
        self.address = address
        self.old_link = old_link

    def run(self) -> None:
        link: PolimidiLink | None = None
        try:
            if self.old_link:
                self.old_link.close()
            link = PolimidiLink(self.address)
            self.finished_ok.emit(
                link,
                link.get_active(),
                link.get_presets(),
                link.get_settings(),
            )
        except Exception as exc:
            if link:
                try:
                    link.close()
                except Exception:
                    pass
            self.finished_err.emit(str(exc))


class ConnectionWatchWorker(QThread):
    """Ping periódico + detecção de queda BLE."""

    lost = pyqtSignal(str)

    def __init__(self, link: PolimidiLink) -> None:
        super().__init__()
        self.link = link
        self._stop = threading.Event()

    def stop(self) -> None:
        self._stop.set()

    def run(self) -> None:
        while not self._stop.is_set():
            if self.link.is_lost() or not self.link.is_connected():
                self.lost.emit("A conexão Bluetooth com o controlador foi encerrada.")
                return
            if not self.link.ping():
                self.lost.emit(
                    "O controlador parou de responder. Verifique se está ligado e por perto."
                )
                return
            if self._stop.wait(PING_INTERVAL_SEC):
                return


class DisconnectWorker(QThread):
    finished_ok = pyqtSignal()
    finished_err = pyqtSignal(str)

    def __init__(self, link: PolimidiLink) -> None:
        super().__init__()
        self.link = link

    def run(self) -> None:
        try:
            self.link.close()
            self.finished_ok.emit()
        except Exception as exc:
            self.finished_err.emit(str(exc))


class SaveSettingsWorker(QThread):
    finished_ok = pyqtSignal()
    finished_err = pyqtSignal(str)

    def __init__(self, link: PolimidiLink, led_brightness: int, midi_clock: bool) -> None:
        super().__init__()
        self.link = link
        self.led_brightness = led_brightness
        self.midi_clock = midi_clock

    def run(self) -> None:
        try:
            self.link.save_settings(self.led_brightness, self.midi_clock)
            self.finished_ok.emit()
        except Exception as exc:
            self.finished_err.emit(str(exc))


class SaveWorker(QThread):
    finished_ok = pyqtSignal(int)
    finished_err = pyqtSignal(str)

    def __init__(self, link: PolimidiLink, preset: int, data: dict) -> None:
        super().__init__()
        self.link = link
        self.preset = preset
        self.data = data

    def run(self) -> None:
        try:
            self.link.save_preset(self.preset, self.data)
            self.finished_ok.emit(self.preset)
        except Exception as exc:
            self.finished_err.emit(str(exc))


class SetActiveWorker(QThread):
    finished_ok = pyqtSignal(int)
    finished_err = pyqtSignal(str)

    def __init__(self, link: PolimidiLink, preset: int) -> None:
        super().__init__()
        self.link = link
        self.preset = preset

    def run(self) -> None:
        try:
            self.link.set_active(self.preset)
            self.finished_ok.emit(self.preset)
        except Exception as exc:
            self.finished_err.emit(str(exc))


class FootPanel(QWidget):
    def __init__(self, foot_label: str) -> None:
        super().__init__()
        self.setObjectName("footPanel")
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(12, 12, 12, 12)
        outer.setSpacing(12)

        title = QLabel(foot_label)
        title.setObjectName("footTitle")
        outer.addWidget(title)

        mode_box = QGroupBox("Modo de acionamento")
        mode_l = QVBoxLayout(mode_box)

        row_name = QHBoxLayout()
        row_name.addWidget(QLabel("Nome"))
        self.name_edit = QLineEdit()
        self.name_edit.setMaxLength(10)
        self.name_edit.setPlaceholderText("Até 10 caracteres")
        row_name.addWidget(self.name_edit, 1)
        mode_l.addLayout(row_name)

        row_mode = QHBoxLayout()
        row_mode.addWidget(QLabel("Modo"))
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["Pressionar", "Tap tempo"])
        self.mode_combo.currentIndexChanged.connect(self._on_mode_change)
        row_mode.addWidget(self.mode_combo)
        row_mode.addStretch()
        mode_l.addLayout(row_mode)
        outer.addWidget(mode_box)

        self.press_box = QGroupBox("Comandos ao pressionar")
        press_l = QVBoxLayout(self.press_box)

        self.radio_unique = QRadioButton("Único — executa a lista A a cada pressão")
        self.radio_toggle = QRadioButton("Alternar — alterna entre lista A e lista B")
        self.radio_unique.setChecked(True)
        self.radio_unique.toggled.connect(self._on_press_change)
        press_l.addWidget(self.radio_unique)
        press_l.addWidget(self.radio_toggle)

        hint = QLabel(
            "No modo único, só a lista A é usada. A lista B fica disponível apenas no modo alternar."
        )
        hint.setObjectName("hintLabel")
        hint.setWordWrap(True)
        press_l.addWidget(hint)

        self.list_a_caption = QLabel("Lista A")
        press_l.addWidget(self.list_a_caption)
        self.list_a = QTextEdit()
        self.list_a.setPlaceholderText("1-PC-1\n1-CC-50-12")
        self.list_a.setFont(QFont("Consolas", 10))
        self.list_a.setMinimumHeight(100)
        press_l.addWidget(self.list_a)

        self._led_a_group = QButtonGroup(self)
        press_l.addWidget(self._make_led_row("LED lista A", self._led_a_group))

        self.list_b_section = QWidget()
        self.list_b_section.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        list_b_l = QVBoxLayout(self.list_b_section)
        list_b_l.setContentsMargins(0, 8, 0, 0)
        list_b_l.addWidget(QLabel("Lista B (2º toque)"))
        self.list_b = QTextEdit()
        self.list_b.setPlaceholderText("1-CC-50-0")
        self.list_b.setFont(QFont("Consolas", 10))
        self.list_b.setMinimumHeight(80)
        list_b_l.addWidget(self.list_b)
        self._led_b_group = QButtonGroup(self)
        list_b_l.addWidget(self._make_led_row("LED lista B", self._led_b_group))
        press_l.addWidget(self.list_b_section)

        outer.addWidget(self.press_box)
        outer.addStretch()
        self._on_mode_change()

    def _make_led_row(self, caption: str, group: QButtonGroup) -> QWidget:
        w = QWidget()
        lay = QHBoxLayout(w)
        lay.setContentsMargins(0, 4, 0, 0)
        lay.addWidget(QLabel(caption))
        for val, lbl in [("off", "Apagado"), ("on", "Aceso"), ("blink", "Piscar")]:
            rb = QRadioButton(lbl)
            rb.setProperty("led_val", val)
            group.addButton(rb)
            lay.addWidget(rb)
            if val == "off":
                rb.setChecked(True)
        lay.addStretch()
        return w

    def _led_value(self, group: QButtonGroup) -> str:
        btn = group.checkedButton()
        return str(btn.property("led_val")) if btn else "off"

    def _set_led_value(self, group: QButtonGroup, val: str) -> None:
        for btn in group.buttons():
            if btn.property("led_val") == val:
                btn.setChecked(True)
                return
        group.buttons()[0].setChecked(True)

    def _on_press_change(self) -> None:
        unique = self.radio_unique.isChecked()
        self.list_b.setEnabled(not unique)
        self.list_b_section.setEnabled(not unique)

    def _on_mode_change(self) -> None:
        is_tap = self.mode_combo.currentIndex() == 1
        if is_tap:
            self.name_edit.setText("TAP TEMPO")
            self.name_edit.setEnabled(False)
        else:
            self.name_edit.setEnabled(True)
        self.press_box.setEnabled(not is_tap)
        if not is_tap:
            self._on_press_change()

    def to_dict(self, tap_already_used: bool) -> tuple[dict, bool]:
        mode = "tap" if self.mode_combo.currentIndex() == 1 else "press"
        if mode == "tap":
            if tap_already_used:
                mode = "press"
            else:
                tap_already_used = True
        name = self.name_edit.text().strip()[:10]
        if mode == "tap":
            name = "TAP TEMPO"
        return (
            {
                "name": name,
                "mode": mode,
                "press": "unique" if self.radio_unique.isChecked() else "toggle",
                "listA": self.list_a.toPlainText().rstrip("\n"),
                "listB": self.list_b.toPlainText().rstrip("\n"),
                "ledA": self._led_value(self._led_a_group),
                "ledB": self._led_value(self._led_b_group),
            },
            tap_already_used,
        )

    def load_dict(self, data: dict, tap_already_used: bool) -> bool:
        mode = data.get("mode", "press")
        if mode == "tap":
            if tap_already_used:
                mode = "press"
            else:
                tap_already_used = True
        self.mode_combo.setCurrentIndex(1 if mode == "tap" else 0)
        if data.get("press") == "toggle":
            self.radio_toggle.setChecked(True)
        else:
            self.radio_unique.setChecked(True)
        self.name_edit.setText("TAP TEMPO" if mode == "tap" else str(data.get("name", ""))[:10])
        self.list_a.setPlainText(data.get("listA", ""))
        self.list_b.setPlainText(data.get("listB", ""))
        self._set_led_value(self._led_a_group, data.get("ledA", "off"))
        self._set_led_value(self._led_b_group, data.get("ledB", "off"))
        self._on_mode_change()
        return tap_already_used


class SettingsPanel(QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.setObjectName("footPanel")
        self.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        outer = QVBoxLayout(self)
        outer.setContentsMargins(12, 12, 12, 12)
        outer.setSpacing(16)

        title = QLabel("Configurações do dispositivo")
        title.setObjectName("footTitle")
        outer.addWidget(title)

        led_box = QGroupBox("Brilho dos LEDs")
        led_l = QVBoxLayout(led_box)
        hint = QLabel("Ajuste o brilho dos LEDs dos pedais (0 a 100%).")
        hint.setObjectName("hintLabel")
        hint.setWordWrap(True)
        led_l.addWidget(hint)

        row = QHBoxLayout()
        self.brightness_slider = QSlider(Qt.Orientation.Horizontal)
        self.brightness_slider.setRange(0, 100)
        self.brightness_slider.setValue(80)
        self.brightness_slider.valueChanged.connect(self._on_brightness_change)
        row.addWidget(self.brightness_slider, 1)
        self.brightness_label = QLabel("80%")
        self.brightness_label.setMinimumWidth(44)
        row.addWidget(self.brightness_label)
        led_l.addLayout(row)
        outer.addWidget(led_box)

        clock_box = QGroupBox("MIDI Clock")
        clock_l = QVBoxLayout(clock_box)
        self.midi_clock_cb = QCheckBox("Sincronizar tempo com o relógio MIDI da pedaleira")
        self.midi_clock_cb.setToolTip(
            "O controlador acompanha o tempo enviado pela pedaleira pelo cabo MIDI. "
            "Com um pedal em Tap tempo, o LED pisca no compasso."
        )
        clock_l.addWidget(self.midi_clock_cb)
        clock_hint = QLabel(
            "Conecte a saída MIDI da pedaleira à entrada do controlador. "
            "O BPM aparece no visor; os comandos CC 74/75 só são enviados no tap manual."
        )
        clock_hint.setObjectName("hintLabel")
        clock_hint.setWordWrap(True)
        clock_l.addWidget(clock_hint)
        outer.addWidget(clock_box)

        self.btn_save_settings = QPushButton("Salvar configurações")
        self.btn_save_settings.setObjectName("primaryBtn")
        self.btn_save_settings.setEnabled(False)
        outer.addWidget(self.btn_save_settings)

        self.settings_status = QLabel("")
        self.settings_status.setObjectName("saveStatus")
        outer.addWidget(self.settings_status)
        outer.addStretch()

    def _on_brightness_change(self, value: int) -> None:
        self.brightness_label.setText(f"{value}%")

    def load(self, settings: dict) -> None:
        led = int(settings.get("ledBrightness", 80))
        led = max(0, min(100, led))
        self.brightness_slider.setValue(led)
        self.brightness_label.setText(f"{led}%")
        self.midi_clock_cb.setChecked(bool(settings.get("midiClock", False)))

    def values(self) -> tuple[int, bool]:
        return self.brightness_slider.value(), self.midi_clock_cb.isChecked()


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle(f"{APP_TITLE} — Configurador")
        self.setMinimumSize(780, 620)
        self.resize(860, 680)

        self._device_map: dict[str, str] = {}
        self.link: PolimidiLink | None = None
        self.presets: dict[int, dict] = {i: empty_preset() for i in range(1, 11)}
        self._workers: list[QThread] = []
        self._preset_changing = False
        self._ui_busy = False
        self._watch_worker: ConnectionWatchWorker | None = None
        self._user_disconnecting = False
        self._connected_label = ""

        root = QWidget()
        root.setObjectName("centralRoot")
        self.setCentralWidget(root)
        layout = QVBoxLayout(root)
        layout.setContentsMargins(16, 16, 16, 12)
        layout.setSpacing(12)

        layout.addWidget(self._build_header())
        layout.addWidget(self._build_preset_bar())
        layout.addWidget(self._build_help())
        layout.addWidget(self._build_tabs(), 1)
        layout.addWidget(self._build_footer())

        sb = QStatusBar()
        self.setStatusBar(sb)
        sb.showMessage("Busque o controlador Polimidi e conecte via Bluetooth")

        self._update_connection_controls()
        self._scan_devices()

    def _build_header(self) -> QFrame:
        bar = QFrame()
        bar.setObjectName("headerBar")
        h = QHBoxLayout(bar)
        h.setContentsMargins(16, 14, 16, 14)

        left = QVBoxLayout()
        left.setSpacing(2)
        t = QLabel(APP_TITLE)
        t.setObjectName("appTitle")
        sub = QLabel("Configurador do controlador MIDI · Bluetooth")
        sub.setObjectName("appSubtitle")
        left.addWidget(t)
        left.addWidget(sub)
        h.addLayout(left, 1)

        self.status_pill = QLabel("Desconectado")
        self.status_pill.setObjectName("statusPill")
        self.status_pill.setProperty("connected", False)
        h.addWidget(self.status_pill, 0, Qt.AlignmentFlag.AlignVCenter)

        h.addSpacing(12)
        h.addWidget(QLabel("Dispositivo"))
        self.device_combo = QComboBox()
        self.device_combo.setMinimumWidth(260)
        h.addWidget(self.device_combo)

        self.btn_scan = QPushButton("Buscar")
        self.btn_scan.clicked.connect(self._scan_devices)
        h.addWidget(self.btn_scan)

        self.btn_connect = QPushButton("Conectar")
        self.btn_connect.clicked.connect(self._connect)
        h.addWidget(self.btn_connect)

        self.btn_disconnect = QPushButton("Desconectar")
        self.btn_disconnect.clicked.connect(self._disconnect)
        self.btn_disconnect.setVisible(False)
        h.addWidget(self.btn_disconnect)
        return bar

    def _build_preset_bar(self) -> QFrame:
        bar = QFrame()
        bar.setObjectName("headerBar")
        h = QHBoxLayout(bar)
        h.setContentsMargins(16, 10, 16, 10)
        h.addWidget(QLabel("Preset ativo"))
        self.preset_combo = QComboBox()
        self.preset_combo.addItems([str(i) for i in range(1, 11)])
        self.preset_combo.setFixedWidth(72)
        self.preset_combo.currentIndexChanged.connect(self._on_preset_change)
        h.addWidget(self.preset_combo)
        self.preset_badge = QLabel("P1")
        self.preset_badge.setObjectName("statusBadge")
        h.addWidget(self.preset_badge)
        h.addStretch()
        return bar

    def _build_help(self) -> QFrame:
        box = QFrame()
        box.setObjectName("helpBox")
        v = QVBoxLayout(box)
        v.setContentsMargins(14, 10, 14, 10)
        title = QLabel("Como escrever comandos")
        title.setObjectName("helpTitle")
        v.addWidget(title)
        body = QLabel(
            "Uma linha por comando, campos separados por travessão (-):<br>"
            "<span style='color:#38bdf8;font-family:Consolas'>1-PC-1</span> — canal 1, troca de programa 1<br>"
            "<span style='color:#38bdf8;font-family:Consolas'>1-CC-50-12</span> — canal 1, controle 50, valor 12"
        )
        body.setObjectName("helpBody")
        body.setTextFormat(Qt.TextFormat.RichText)
        body.setWordWrap(True)
        v.addWidget(body)
        return box

    def _build_tabs(self) -> QTabWidget:
        self.tabs = QTabWidget()
        self.foot_panels: list[FootPanel] = []
        for lbl in ["Pedal A", "Pedal B", "Pedal C", "Pedal D"]:
            scroll = QScrollArea()
            scroll.setObjectName("footScroll")
            scroll.setWidgetResizable(True)
            scroll.setFrameShape(QFrame.Shape.NoFrame)
            scroll.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
            scroll.viewport().setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
            panel = FootPanel(lbl)
            scroll.setWidget(panel)
            self.foot_panels.append(panel)
            self.tabs.addTab(scroll, lbl)

        scroll = QScrollArea()
        scroll.setObjectName("footScroll")
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.Shape.NoFrame)
        scroll.setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        scroll.viewport().setAttribute(Qt.WidgetAttribute.WA_StyledBackground, True)
        self.settings_panel = SettingsPanel()
        scroll.setWidget(self.settings_panel)
        self.tabs.addTab(scroll, "Configurações")
        self.settings_panel.btn_save_settings.clicked.connect(self._save_settings)
        return self.tabs

    def _build_footer(self) -> QFrame:
        bar = QFrame()
        bar.setObjectName("footerBar")
        h = QHBoxLayout(bar)
        h.setContentsMargins(16, 12, 16, 12)
        self.btn_save = QPushButton("Salvar preset")
        self.btn_save.setObjectName("primaryBtn")
        self.btn_save.setEnabled(False)
        self.btn_save.clicked.connect(self._save)
        h.addWidget(self.btn_save)
        self.save_label = QLabel("")
        self.save_label.setObjectName("saveStatus")
        h.addWidget(self.save_label, 1)
        return bar

    def _set_connected_ui(self, connected: bool, text: str) -> None:
        self.status_pill.setText(text)
        self.status_pill.setProperty("connected", connected)
        self.status_pill.style().unpolish(self.status_pill)
        self.status_pill.style().polish(self.status_pill)

    def _update_connection_controls(self) -> None:
        connected = self.link is not None
        busy = self._ui_busy
        self.device_combo.setEnabled(not connected and not busy)
        self.btn_scan.setEnabled(not connected and not busy)
        self.btn_connect.setVisible(not connected)
        self.btn_connect.setEnabled(not connected and not busy)
        self.btn_disconnect.setVisible(connected)
        self.btn_disconnect.setEnabled(connected and not busy)
        self.btn_save.setEnabled(connected and not busy)
        self.settings_panel.btn_save_settings.setEnabled(connected and not busy)

    def _set_busy(self, busy: bool) -> None:
        self._ui_busy = busy
        self._update_connection_controls()

    def _stop_watch(self) -> None:
        if self._watch_worker is None:
            return
        self._watch_worker.stop()
        if self._watch_worker.isRunning():
            self._watch_worker.wait(3000)
        self._watch_worker = None

    def _start_watch(self, link: PolimidiLink) -> None:
        self._stop_watch()
        worker = ConnectionWatchWorker(link)
        worker.lost.connect(self._on_connection_lost)
        self._watch_worker = worker
        self._track_worker(worker)
        worker.start()

    def _reset_disconnected_state(self, status: str = "Desconectado") -> None:
        self.link = None
        self._connected_label = ""
        self.btn_save.setEnabled(False)
        self.settings_panel.btn_save_settings.setEnabled(False)
        self.save_label.setText("")
        self.save_label.setObjectName("saveStatus")
        self.settings_panel.settings_status.setText("")
        self._set_connected_ui(False, status)
        self._update_connection_controls()
        self.statusBar().showMessage("Desconectado — busque o controlador e conecte novamente")

    def _show_error(self, message: str) -> None:
        QMessageBox.critical(self, APP_TITLE, friendly_error(message))

    def _show_warning(self, message: str) -> None:
        QMessageBox.warning(self, APP_TITLE, friendly_error(message))

    def _on_connection_lost(self, msg: str) -> None:
        if self._user_disconnecting:
            return
        self._stop_watch()
        if self.link:
            try:
                self.link.close()
            except Exception:
                pass
        self._reset_disconnected_state("Desconectado")
        self._show_warning(
            f"{msg}\n\nClique em Buscar e conecte novamente ao {APP_TITLE}."
        )

    def _disconnect(self) -> None:
        if not self.link:
            return
        self._user_disconnecting = True
        self._set_busy(True)
        self._stop_watch()
        link = self.link
        self.link = None
        self._update_connection_controls()
        worker = DisconnectWorker(link)
        worker.finished_ok.connect(self._on_disconnect_ok)
        worker.finished_err.connect(self._on_disconnect_err)
        worker.finished.connect(self._on_disconnect_finished)
        self._track_worker(worker)
        worker.start()

    def _on_disconnect_ok(self) -> None:
        self._reset_disconnected_state("Desconectado")
        self.statusBar().showMessage("Desconectado")

    def _on_disconnect_err(self, msg: str) -> None:
        self._reset_disconnected_state("Desconectado")
        self._show_warning(f"Não foi possível encerrar a conexão:\n{friendly_error(msg)}")

    def _on_disconnect_finished(self) -> None:
        self._user_disconnecting = False
        self._set_busy(False)

    def _track_worker(self, worker: QThread) -> None:
        self._workers.append(worker)

        def cleanup() -> None:
            if worker in self._workers:
                self._workers.remove(worker)

        worker.finished.connect(cleanup)

    def _scan_devices(self) -> None:
        self._set_busy(True)
        self._set_connected_ui(False, "Buscando…")
        worker = ScanWorker()
        worker.finished_ok.connect(self._on_scan_ok)
        worker.finished_err.connect(self._on_scan_err)
        worker.finished.connect(lambda: self._set_busy(False))
        self._track_worker(worker)
        worker.start()

    def _on_scan_ok(self, devices: list) -> None:
        if self.link is not None:
            return
        labels = [f"{name} ({addr})" for addr, name in devices]
        self._device_map = {labels[i]: devices[i][0] for i in range(len(labels))}
        self.device_combo.clear()
        self.device_combo.addItems(labels)
        self._set_connected_ui(False, f"{len(labels)} encontrado(s)" if labels else "Nenhum encontrado")

    def _on_scan_err(self, msg: str) -> None:
        self._set_connected_ui(False, "Erro na busca")
        self._show_error(f"Não foi possível buscar dispositivos Bluetooth:\n{msg}")

    def _connect(self) -> None:
        if self.link is not None:
            return
        label = self.device_combo.currentText().strip()
        if not label or label not in self._device_map:
            self._show_warning("Selecione um controlador na lista.\nSe a lista estiver vazia, clique em Buscar.")
            return
        address = self._device_map[label]
        self._set_busy(True)
        self._set_connected_ui(False, "Conectando…")
        worker = ConnectWorker(address, self.link)
        worker.finished_ok.connect(
            lambda link, active, stored, settings: self._on_connect_ok(
                link, active, stored, settings, label
            )
        )
        worker.finished_err.connect(self._on_connect_err)
        worker.finished.connect(lambda: self._set_busy(False))
        self._track_worker(worker)
        worker.start()

    def _on_connect_ok(
        self,
        link: PolimidiLink,
        active: int,
        stored: dict,
        settings: dict,
        label: str,
    ) -> None:
        self.link = link
        self._connected_label = label
        self._preset_changing = True
        self.preset_combo.setCurrentIndex(active - 1)
        self.preset_badge.setText(f"P{active}")
        self._preset_changing = False
        for n in range(1, 11):
            if stored.get(n):
                self.presets[n] = stored[n]
        self._load_form_from_cache()
        self.settings_panel.load(settings)
        self.settings_panel.settings_status.setText("")
        short = label.split(" (")[0]
        self._set_connected_ui(True, f"Conectado · {short}")
        self.save_label.setText("")
        self.save_label.setObjectName("saveStatus")
        self._update_connection_controls()
        self._start_watch(link)
        self.statusBar().showMessage("Conectado — presets e configurações carregados")

    def _on_connect_err(self, msg: str) -> None:
        self._reset_disconnected_state("Desconectado")
        self._show_error(msg)

    def _cache_current_form(self) -> None:
        tap_used = False
        feet = []
        for fp in self.foot_panels:
            foot, tap_used = fp.to_dict(tap_used)
            feet.append(foot)
        preset = self.preset_combo.currentIndex() + 1
        self.presets[preset] = {"feet": feet}

    def _load_form_from_cache(self) -> None:
        preset = self.preset_combo.currentIndex() + 1
        data = self.presets.get(preset, empty_preset())
        feet = data.get("feet", [])
        tap_used = False
        for i, fp in enumerate(self.foot_panels):
            foot = feet[i] if i < len(feet) else empty_foot()
            tap_used = fp.load_dict(foot, tap_used)

    def _on_preset_change(self) -> None:
        if self._preset_changing:
            return
        new_n = self.preset_combo.currentIndex() + 1
        self.preset_badge.setText(f"P{new_n}")
        self._cache_current_form()
        if not self.link:
            self._load_form_from_cache()
            return
        self._set_busy(True)
        worker = SetActiveWorker(self.link, new_n)
        worker.finished_ok.connect(self._on_set_active_ok)
        worker.finished_err.connect(self._on_set_active_err)
        worker.finished.connect(lambda: self._set_busy(False))
        self._track_worker(worker)
        worker.start()

    def _on_set_active_ok(self, n: int) -> None:
        self._load_form_from_cache()
        self.save_label.setText(f"Preset {n} ativo")
        self.save_label.setObjectName("saveStatus")

    def _on_set_active_err(self, msg: str) -> None:
        self._show_error(msg)

    def _save(self) -> None:
        if not self.link:
            self._show_warning(f"Conecte-se ao {APP_TITLE} antes de salvar o preset.")
            return
        self._cache_current_form()
        n = self.preset_combo.currentIndex() + 1
        self._set_busy(True)
        self.save_label.setText("Salvando…")
        self.save_label.setObjectName("saveStatus")
        worker = SaveWorker(self.link, n, self.presets[n])
        worker.finished_ok.connect(self._on_save_ok)
        worker.finished_err.connect(self._on_save_err)
        worker.finished.connect(lambda: self._set_busy(False))
        self._track_worker(worker)
        worker.start()

    def _on_save_ok(self, n: int) -> None:
        self.save_label.setText(f"Preset {n} salvo ✓")
        self.save_label.setObjectName("saveStatusOk")
        self.statusBar().showMessage(f"Preset {n} gravado no controlador")

    def _on_save_err(self, msg: str) -> None:
        self.save_label.setText("")
        self.save_label.setObjectName("saveStatus")
        self._show_error(msg)

    def _save_settings(self) -> None:
        if not self.link:
            self._show_warning(f"Conecte-se ao {APP_TITLE} antes de salvar o preset.")
            return
        led, clk = self.settings_panel.values()
        self._set_busy(True)
        self.settings_panel.settings_status.setText("Salvando…")
        self.settings_panel.settings_status.setObjectName("saveStatus")
        worker = SaveSettingsWorker(self.link, led, clk)
        worker.finished_ok.connect(self._on_save_settings_ok)
        worker.finished_err.connect(self._on_save_settings_err)
        worker.finished.connect(lambda: self._set_busy(False))
        self._track_worker(worker)
        worker.start()

    def _on_save_settings_ok(self) -> None:
        self.settings_panel.settings_status.setText("Configurações salvas ✓")
        self.settings_panel.settings_status.setObjectName("saveStatusOk")
        self.statusBar().showMessage("Configurações gravadas no controlador")

    def _on_save_settings_err(self, msg: str) -> None:
        self.settings_panel.settings_status.setText("")
        self.settings_panel.settings_status.setObjectName("saveStatus")
        self._show_error(msg)

    def closeEvent(self, event) -> None:
        self._user_disconnecting = True
        self._stop_watch()
        if self.link:
            try:
                self.link.close()
            except Exception:
                pass
            self.link = None
        for w in list(self._workers):
            if w.isRunning():
                w.wait(2000)
        event.accept()


def _apply_dark_palette(app: QApplication) -> None:
    p = QPalette()
    bg = QColor("#0f1419")
    panel = QColor("#1a2332")
    text = QColor("#e2e8f0")
    muted = QColor("#94a3b8")
    input_bg = QColor("#0f1419")
    accent = QColor("#38bdf8")

    p.setColor(QPalette.ColorRole.Window, bg)
    p.setColor(QPalette.ColorRole.WindowText, text)
    p.setColor(QPalette.ColorRole.Base, input_bg)
    p.setColor(QPalette.ColorRole.AlternateBase, panel)
    p.setColor(QPalette.ColorRole.ToolTipBase, panel)
    p.setColor(QPalette.ColorRole.ToolTipText, text)
    p.setColor(QPalette.ColorRole.Text, text)
    p.setColor(QPalette.ColorRole.Button, QColor("#1e293b"))
    p.setColor(QPalette.ColorRole.ButtonText, text)
    p.setColor(QPalette.ColorRole.BrightText, accent)
    p.setColor(QPalette.ColorRole.Link, accent)
    p.setColor(QPalette.ColorRole.Highlight, accent)
    p.setColor(QPalette.ColorRole.HighlightedText, QColor("#0f172a"))
    p.setColor(QPalette.ColorRole.PlaceholderText, muted)
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.Text, QColor("#64748b"))
    p.setColor(QPalette.ColorGroup.Disabled, QPalette.ColorRole.WindowText, QColor("#64748b"))
    app.setPalette(p)


def main() -> None:
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    _apply_dark_palette(app)
    app.setStyleSheet(STYLESHEET)
    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
