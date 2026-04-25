#ifndef WELCOME_PAGE_H
#define WELCOME_PAGE_H

#include <Arduino.h>

const char WELCOME_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Controlador MIDI — Configuração</title>
  <style>
    :root {
      color-scheme: light dark;
      --bg: #0f1419;
      --surface: #1a2332;
      --border: #334155;
      --text: #e2e8f0;
      --muted: #94a3b8;
      --accent: #38bdf8;
      --accent-dim: rgba(56, 189, 248, 0.15);
      --radius: 10px;
      --font: system-ui, -apple-system, "Segoe UI", sans-serif;
    }
    @media (prefers-color-scheme: light) {
      :root {
        --bg: #f8fafc;
        --surface: #fff;
        --border: #cbd5e1;
        --text: #0f172a;
        --muted: #64748b;
        --accent-dim: rgba(14, 165, 233, 0.12);
      }
    }
    * { box-sizing: border-box; }
    body {
      font-family: var(--font);
      margin: 0;
      min-height: 100vh;
      background: var(--bg);
      color: var(--text);
      line-height: 1.5;
      padding: 1rem 1rem 2.5rem;
    }
    .wrap { max-width: 40rem; margin: 0 auto; }
    header { margin-bottom: 1rem; }
    header h1 { font-size: 1.25rem; font-weight: 600; margin: 0 0 0.25rem; }
    header p { margin: 0; font-size: 0.875rem; color: var(--muted); }

    .preset-bar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.5rem 1rem;
      margin-bottom: 1rem;
      padding: 0.75rem 1rem;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius);
    }
    .preset-bar label { font-size: 0.875rem; font-weight: 600; }
    .preset-bar select {
      font: inherit;
      font-size: 0.875rem;
      padding: 0.45rem 0.65rem;
      border-radius: 8px;
      border: 1px solid var(--border);
      background: var(--bg);
      color: var(--text);
      min-width: 5rem;
    }

    .format-help {
      font-size: 0.8125rem;
      color: var(--muted);
      padding: 0.85rem 1rem;
      margin-bottom: 1rem;
      border: 1px solid var(--border);
      border-radius: var(--radius);
      background: var(--accent-dim);
    }
    .format-help code {
      font-family: ui-monospace, monospace;
      font-size: 0.8rem;
      color: var(--text);
      display: block;
      margin: 0.35rem 0;
    }
    .format-help strong { color: var(--text); }

    .tabs {
      display: flex;
      flex-wrap: wrap;
      gap: 0.35rem;
      margin-bottom: 1rem;
      border-bottom: 1px solid var(--border);
      padding-bottom: 0.5rem;
    }
    .tabs button {
      font: inherit;
      font-size: 0.875rem;
      font-weight: 500;
      padding: 0.5rem 0.85rem;
      border: 1px solid transparent;
      border-radius: 8px;
      background: transparent;
      color: var(--muted);
      cursor: pointer;
    }
    .tabs button:hover { color: var(--text); background: var(--accent-dim); }
    .tabs button[aria-selected="true"] {
      color: var(--accent);
      background: var(--accent-dim);
      border-color: var(--accent);
    }

    .panel {
      display: none;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 1.1rem 1.15rem 1.25rem;
    }
    .panel.active { display: block; }

    .panel h2 {
      font-size: 0.7rem;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--muted);
      margin: 0 0 1rem;
      font-weight: 600;
    }

    fieldset {
      border: 1px solid var(--border);
      border-radius: 8px;
      margin: 0 0 1rem;
      padding: 0.85rem 1rem;
    }
    fieldset:last-child { margin-bottom: 0; }
    legend {
      font-size: 0.75rem;
      font-weight: 600;
      color: var(--muted);
      padding: 0 0.35rem;
    }

    .row { margin-bottom: 0.75rem; }
    .row:last-child { margin-bottom: 0; }
    label.block {
      display: block;
      font-size: 0.8125rem;
      font-weight: 500;
      margin-bottom: 0.35rem;
    }
    select.mode-select, textarea {
      width: 100%;
      font: inherit;
      font-size: 0.875rem;
      color: var(--text);
      background: var(--bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 0.5rem 0.6rem;
    }
    textarea {
      min-height: 6rem;
      resize: vertical;
      font-family: ui-monospace, monospace;
      line-height: 1.45;
    }
    select.mode-select { cursor: pointer; }

    .radio-group {
      display: flex;
      flex-direction: column;
      gap: 0.5rem;
    }
    .radio-group label.choice,
    .led-row label.choice {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      font-size: 0.875rem;
      font-weight: 400;
      cursor: pointer;
    }
    .radio-group input,
    .led-row input { accent-color: var(--accent); }

    .mode-hint {
      font-size: 0.8125rem;
      color: var(--muted);
      padding: 0.65rem 0.75rem;
      margin-top: 0.75rem;
      border: 1px dashed var(--border);
      border-radius: 8px;
      background: var(--accent-dim);
    }

    .list-block {
      margin-bottom: 1rem;
      padding-bottom: 1rem;
      border-bottom: 1px solid var(--border);
    }
    .list-block:last-child {
      margin-bottom: 0;
      padding-bottom: 0;
      border-bottom: none;
    }
    .list-block h3 {
      font-size: 0.8125rem;
      font-weight: 600;
      margin: 0 0 0.35rem;
    }
    .list-block .sub {
      font-size: 0.75rem;
      color: var(--muted);
      margin: 0 0 0.5rem;
    }

    .foot-panel.unique-mode .list-b-wrap {
      display: none !important;
      visibility: hidden;
      max-height: 0;
      overflow: hidden;
      margin: 0 !important;
      padding: 0 !important;
      border: none !important;
    }
    .foot-panel.unique-mode .list-a-sub-unique { display: block; }
    .foot-panel:not(.unique-mode) .list-a-sub-unique { display: none; }
    .foot-panel.unique-mode .list-a-sub-toggle { display: none; }
    .foot-panel:not(.unique-mode) .list-a-sub-toggle { display: block; }

    .led-row {
      margin-top: 0.65rem;
      display: flex;
      flex-wrap: wrap;
      gap: 1rem;
    }
    .led-row > span {
      font-size: 0.75rem;
      font-weight: 600;
      color: var(--muted);
      width: 100%;
    }

    .save-bar {
      margin-top: 1.25rem;
      padding: 1rem;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.75rem 1rem;
    }
    .save-bar button {
      font: inherit;
      font-size: 0.9rem;
      font-weight: 600;
      padding: 0.6rem 1.25rem;
      border: none;
      border-radius: 8px;
      background: var(--accent);
      color: #0f172a;
      cursor: pointer;
    }
    .save-bar button:hover { filter: brightness(1.08); }
    .save-bar button:disabled { opacity: 0.6; cursor: wait; }
    #save-status { font-size: 0.875rem; color: var(--muted); }
    #save-status.ok { color: #22c55e; }
    #save-status.err { color: #f87171; }

    footer {
      margin-top: 1.5rem;
      text-align: center;
      font-size: 0.75rem;
      color: var(--muted);
    }
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <h1>Controlador MIDI</h1>
      <p>10 presets independentes · cada foot (A–D) com sua configuração</p>
    </header>

    <div class="preset-bar">
      <label for="preset-select">Preset</label>
      <select id="preset-select" aria-label="Número do preset">
        <option value="1">1</option>
        <option value="2">2</option>
        <option value="3">3</option>
        <option value="4">4</option>
        <option value="5">5</option>
        <option value="6">6</option>
        <option value="7">7</option>
        <option value="8">8</option>
        <option value="9">9</option>
        <option value="10">10</option>
      </select>
    </div>

    <div class="format-help">
      <strong>Formato dos comandos</strong> — uma linha por comando, campos separados por <strong>travessão</strong> (-):
      <code>canal-tipo-parâmetros</code>
      <code>1-PC-1</code> → canal 1, Program Change, programa 1
      <code>1-CC-50-12</code> → canal 1, Control Change, número 50, valor 12<br>
      Tipos suportados por enquanto: <strong>PC</strong> e <strong>CC</strong>.
    </div>

    <div class="tabs" role="tablist" aria-label="Foots">
      <button type="button" role="tab" id="tab-a" aria-selected="true" aria-controls="panel-a" data-panel="panel-a">Foot A</button>
      <button type="button" role="tab" id="tab-b" aria-selected="false" aria-controls="panel-b" data-panel="panel-b">Foot B</button>
      <button type="button" role="tab" id="tab-c" aria-selected="false" aria-controls="panel-c" data-panel="panel-c">Foot C</button>
      <button type="button" role="tab" id="tab-d" aria-selected="false" aria-controls="panel-d" data-panel="panel-d">Foot D</button>
    </div>

    <section id="panel-a" class="panel foot-panel unique-mode active" role="tabpanel" aria-labelledby="tab-a" data-foot="0">
      <h2>Foot A</h2>
      <fieldset>
        <legend>Modo de acionamento</legend>
        <div class="row">
          <label class="block" for="f0-mode">Modo</label>
          <select id="f0-mode" class="mode-select" disabled><option selected>Press</option></select>
        </div>
      </fieldset>
      <fieldset>
        <legend>No modo Press</legend>
        <div class="radio-group">
          <label class="choice"><input type="radio" name="foot0_press" value="unique" checked> Único (unique)</label>
          <label class="choice"><input type="radio" name="foot0_press" value="toggle"> Alternar (toggle)</label>
        </div>
        <p class="mode-hint">No modo <strong>único</strong>, só a <strong>lista A</strong> é usada — enviada a cada pressão. A <strong>lista B</strong> e o LED dela aparecem apenas no modo <strong>alternar</strong>.</p>
        <div class="list-block">
          <h3>Lista A</h3>
          <p class="sub list-a-sub-unique">Comandos enviados <strong>a cada</strong> pressão.</p>
          <p class="sub list-a-sub-toggle">Comandos do <strong>1º</strong> toque (modo alternar).</p>
          <label class="block" for="f0-list-a">Uma linha por comando</label>
          <textarea id="f0-list-a" name="f0_list_a" placeholder="1-PC-1&#10;1-CC-50-12"></textarea>
          <div class="led-row" role="group" aria-label="LED lista A">
            <span class="led-caption-a">LED (lista A)</span>
            <label class="choice"><input type="radio" name="foot0_led_a" value="off" checked> Desligado</label>
            <label class="choice"><input type="radio" name="foot0_led_a" value="on"> Ligado</label>
            <label class="choice"><input type="radio" name="foot0_led_a" value="blink"> Piscar</label>
          </div>
        </div>
        <div class="list-b-wrap">
          <div class="list-block">
            <h3>Lista B</h3>
            <p class="sub">Comandos do <strong>2º</strong> toque (só modo alternar).</p>
            <label class="block" for="f0-list-b">Uma linha por comando</label>
            <textarea id="f0-list-b" name="f0_list_b" placeholder="1-CC-50-0"></textarea>
            <div class="led-row" role="group" aria-label="LED lista B">
              <span>LED (lista B)</span>
              <label class="choice"><input type="radio" name="foot0_led_b" value="off" checked> Desligado</label>
              <label class="choice"><input type="radio" name="foot0_led_b" value="on"> Ligado</label>
              <label class="choice"><input type="radio" name="foot0_led_b" value="blink"> Piscar</label>
            </div>
          </div>
        </div>
      </fieldset>
    </section>

    <section id="panel-b" class="panel foot-panel unique-mode" role="tabpanel" aria-labelledby="tab-b" data-foot="1" hidden>
      <h2>Foot B</h2>
      <fieldset>
        <legend>Modo de acionamento</legend>
        <div class="row">
          <label class="block" for="f1-mode">Modo</label>
          <select id="f1-mode" class="mode-select" disabled><option selected>Press</option></select>
        </div>
      </fieldset>
      <fieldset>
        <legend>No modo Press</legend>
        <div class="radio-group">
          <label class="choice"><input type="radio" name="foot1_press" value="unique" checked> Único (unique)</label>
          <label class="choice"><input type="radio" name="foot1_press" value="toggle"> Alternar (toggle)</label>
        </div>
        <p class="mode-hint">No modo <strong>único</strong>, só a <strong>lista A</strong> é usada — enviada a cada pressão. A <strong>lista B</strong> e o LED dela aparecem apenas no modo <strong>alternar</strong>.</p>
        <div class="list-block">
          <h3>Lista A</h3>
          <p class="sub list-a-sub-unique">Comandos enviados <strong>a cada</strong> pressão.</p>
          <p class="sub list-a-sub-toggle">Comandos do <strong>1º</strong> toque (modo alternar).</p>
          <label class="block" for="f1-list-a">Uma linha por comando</label>
          <textarea id="f1-list-a" name="f1_list_a" placeholder="1-PC-1&#10;1-CC-50-12"></textarea>
          <div class="led-row" role="group" aria-label="LED lista A">
            <span class="led-caption-a">LED (lista A)</span>
            <label class="choice"><input type="radio" name="foot1_led_a" value="off" checked> Desligado</label>
            <label class="choice"><input type="radio" name="foot1_led_a" value="on"> Ligado</label>
            <label class="choice"><input type="radio" name="foot1_led_a" value="blink"> Piscar</label>
          </div>
        </div>
        <div class="list-b-wrap">
          <div class="list-block">
            <h3>Lista B</h3>
            <p class="sub">Comandos do <strong>2º</strong> toque (só modo alternar).</p>
            <label class="block" for="f1-list-b">Uma linha por comando</label>
            <textarea id="f1-list-b" name="f1_list_b" placeholder="1-CC-50-0"></textarea>
            <div class="led-row" role="group" aria-label="LED lista B">
              <span>LED (lista B)</span>
              <label class="choice"><input type="radio" name="foot1_led_b" value="off" checked> Desligado</label>
              <label class="choice"><input type="radio" name="foot1_led_b" value="on"> Ligado</label>
              <label class="choice"><input type="radio" name="foot1_led_b" value="blink"> Piscar</label>
            </div>
          </div>
        </div>
      </fieldset>
    </section>

    <section id="panel-c" class="panel foot-panel unique-mode" role="tabpanel" aria-labelledby="tab-c" data-foot="2" hidden>
      <h2>Foot C</h2>
      <fieldset>
        <legend>Modo de acionamento</legend>
        <div class="row">
          <label class="block" for="f2-mode">Modo</label>
          <select id="f2-mode" class="mode-select" disabled><option selected>Press</option></select>
        </div>
      </fieldset>
      <fieldset>
        <legend>No modo Press</legend>
        <div class="radio-group">
          <label class="choice"><input type="radio" name="foot2_press" value="unique" checked> Único (unique)</label>
          <label class="choice"><input type="radio" name="foot2_press" value="toggle"> Alternar (toggle)</label>
        </div>
        <p class="mode-hint">No modo <strong>único</strong>, só a <strong>lista A</strong> é usada — enviada a cada pressão. A <strong>lista B</strong> e o LED dela aparecem apenas no modo <strong>alternar</strong>.</p>
        <div class="list-block">
          <h3>Lista A</h3>
          <p class="sub list-a-sub-unique">Comandos enviados <strong>a cada</strong> pressão.</p>
          <p class="sub list-a-sub-toggle">Comandos do <strong>1º</strong> toque (modo alternar).</p>
          <label class="block" for="f2-list-a">Uma linha por comando</label>
          <textarea id="f2-list-a" name="f2_list_a" placeholder="1-PC-1&#10;1-CC-50-12"></textarea>
          <div class="led-row" role="group" aria-label="LED lista A">
            <span class="led-caption-a">LED (lista A)</span>
            <label class="choice"><input type="radio" name="foot2_led_a" value="off" checked> Desligado</label>
            <label class="choice"><input type="radio" name="foot2_led_a" value="on"> Ligado</label>
            <label class="choice"><input type="radio" name="foot2_led_a" value="blink"> Piscar</label>
          </div>
        </div>
        <div class="list-b-wrap">
          <div class="list-block">
            <h3>Lista B</h3>
            <p class="sub">Comandos do <strong>2º</strong> toque (só modo alternar).</p>
            <label class="block" for="f2-list-b">Uma linha por comando</label>
            <textarea id="f2-list-b" name="f2_list_b" placeholder="1-CC-50-0"></textarea>
            <div class="led-row" role="group" aria-label="LED lista B">
              <span>LED (lista B)</span>
              <label class="choice"><input type="radio" name="foot2_led_b" value="off" checked> Desligado</label>
              <label class="choice"><input type="radio" name="foot2_led_b" value="on"> Ligado</label>
              <label class="choice"><input type="radio" name="foot2_led_b" value="blink"> Piscar</label>
            </div>
          </div>
        </div>
      </fieldset>
    </section>

    <section id="panel-d" class="panel foot-panel unique-mode" role="tabpanel" aria-labelledby="tab-d" data-foot="3" hidden>
      <h2>Foot D</h2>
      <fieldset>
        <legend>Modo de acionamento</legend>
        <div class="row">
          <label class="block" for="f3-mode">Modo</label>
          <select id="f3-mode" class="mode-select" disabled><option selected>Press</option></select>
        </div>
      </fieldset>
      <fieldset>
        <legend>No modo Press</legend>
        <div class="radio-group">
          <label class="choice"><input type="radio" name="foot3_press" value="unique" checked> Único (unique)</label>
          <label class="choice"><input type="radio" name="foot3_press" value="toggle"> Alternar (toggle)</label>
        </div>
        <p class="mode-hint">No modo <strong>único</strong>, só a <strong>lista A</strong> é usada — enviada a cada pressão. A <strong>lista B</strong> e o LED dela aparecem apenas no modo <strong>alternar</strong>.</p>
        <div class="list-block">
          <h3>Lista A</h3>
          <p class="sub list-a-sub-unique">Comandos enviados <strong>a cada</strong> pressão.</p>
          <p class="sub list-a-sub-toggle">Comandos do <strong>1º</strong> toque (modo alternar).</p>
          <label class="block" for="f3-list-a">Uma linha por comando</label>
          <textarea id="f3-list-a" name="f3_list_a" placeholder="1-PC-1&#10;1-CC-50-12"></textarea>
          <div class="led-row" role="group" aria-label="LED lista A">
            <span class="led-caption-a">LED (lista A)</span>
            <label class="choice"><input type="radio" name="foot3_led_a" value="off" checked> Desligado</label>
            <label class="choice"><input type="radio" name="foot3_led_a" value="on"> Ligado</label>
            <label class="choice"><input type="radio" name="foot3_led_a" value="blink"> Piscar</label>
          </div>
        </div>
        <div class="list-b-wrap">
          <div class="list-block">
            <h3>Lista B</h3>
            <p class="sub">Comandos do <strong>2º</strong> toque (só modo alternar).</p>
            <label class="block" for="f3-list-b">Uma linha por comando</label>
            <textarea id="f3-list-b" name="f3_list_b" placeholder="1-CC-50-0"></textarea>
            <div class="led-row" role="group" aria-label="LED lista B">
              <span>LED (lista B)</span>
              <label class="choice"><input type="radio" name="foot3_led_b" value="off" checked> Desligado</label>
              <label class="choice"><input type="radio" name="foot3_led_b" value="on"> Ligado</label>
              <label class="choice"><input type="radio" name="foot3_led_b" value="blink"> Piscar</label>
            </div>
          </div>
        </div>
      </fieldset>
    </section>

    <div class="save-bar">
      <button type="button" id="btn-save">Salvar preset</button>
      <span id="save-status" role="status"></span>
    </div>

    <footer>Controlador MIDI · rede local</footer>
  </div>

  <script>
    (function () {
      var PANEL_IDS = ['panel-a', 'panel-b', 'panel-c', 'panel-d'];
      var emptyFoot = function () {
        return { press: 'unique', listA: '', listB: '', ledA: 'off', ledB: 'off' };
      };
      var emptyPreset = function () {
        return { feet: [emptyFoot(), emptyFoot(), emptyFoot(), emptyFoot()] };
      };
      var presets = {};
      for (var p = 1; p <= 10; p++) presets[p] = emptyPreset();
      var currentPreset = 1;

      function footPanelEl(i) {
        return document.getElementById(PANEL_IDS[i]);
      }

      function updateFootUi(i) {
        var panel = footPanelEl(i);
        if (!panel) return;
        var u = document.querySelector('input[name="foot' + i + '_press"][value="unique"]');
        var unique = u && u.checked;
        panel.classList.toggle('unique-mode', unique);
        var cap = panel.querySelector('.led-caption-a');
        if (cap) cap.textContent = unique ? 'LED (após cada pressão)' : 'LED (lista A — 1º toque)';
        var tb = document.getElementById('f' + i + '-list-b');
        if (tb) tb.disabled = unique;
      }

      function updateAllFeetUi() {
        for (var i = 0; i < 4; i++) updateFootUi(i);
      }

      function normLedJson(v, defVal) {
        if (v === 'off') return 'off';
        if (v === 'blink' || v === 'pisca') return 'blink';
        if (v === 'on') return 'on';
        return defVal || 'off';
      }

      function readPresetFromForm() {
        var feet = [];
        for (var i = 0; i < 4; i++) {
          var pr = document.querySelector('input[name="foot' + i + '_press"]:checked');
          feet.push({
            press: pr ? pr.value : 'unique',
            listA: (document.getElementById('f' + i + '-list-a') || {}).value || '',
            listB: (document.getElementById('f' + i + '-list-b') || {}).value || '',
            ledA: normLedJson((document.querySelector('input[name="foot' + i + '_led_a"]:checked') || {}).value, 'off'),
            ledB: normLedJson((document.querySelector('input[name="foot' + i + '_led_b"]:checked') || {}).value, 'off')
          });
        }
        return { feet: feet };
      }

      function setRadio(name, val) {
        var el = document.querySelector('input[name="' + name + '"][value="' + val + '"]');
        if (el) el.checked = true;
      }

      function applyPresetToForm(data) {
        var feet = (data && data.feet) ? data.feet : [];
        for (var i = 0; i < 4; i++) {
          var f = feet[i] || emptyFoot();
          var press = f.press === 'unique' ? 'unique' : 'toggle';
          var pu = document.querySelector('input[name="foot' + i + '_press"][value="' + press + '"]');
          if (pu) pu.checked = true;
          var la = document.getElementById('f' + i + '-list-a');
          var lb = document.getElementById('f' + i + '-list-b');
          if (la) la.value = f.listA || '';
          if (lb) lb.value = f.listB || '';
          setRadio('foot' + i + '_led_a', normLedJson(f.ledA, 'off'));
          setRadio('foot' + i + '_led_b', normLedJson(f.ledB, 'off'));
          updateFootUi(i);
        }
      }

      function bindTabs() {
        var tabs = document.querySelectorAll('.tabs [role="tab"]');
        var panels = document.querySelectorAll('.panel');
        tabs.forEach(function (tab) {
          tab.addEventListener('click', function () {
            var id = tab.getAttribute('data-panel');
            tabs.forEach(function (t) { t.setAttribute('aria-selected', t === tab ? 'true' : 'false'); });
            panels.forEach(function (p) {
              var on = p.id === id;
              p.classList.toggle('active', on);
              p.hidden = !on;
            });
          });
        });
      }

      function bindPressRadios() {
        for (var fi = 0; fi < 4; fi++) {
          (function (footIdx) {
            document.querySelectorAll('input[name="foot' + footIdx + '_press"]').forEach(function (r) {
              r.addEventListener('change', function () { updateFootUi(footIdx); });
            });
          })(fi);
        }
      }

      var sel = document.getElementById('preset-select');
      sel.addEventListener('change', function () {
        var old = currentPreset;
        presets[old] = readPresetFromForm();
        currentPreset = parseInt(sel.value, 10);
        applyPresetToForm(presets[currentPreset]);
        var st = document.getElementById('save-status');
        st.textContent = '';
        st.className = '';
        fetch('/setActive', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'active=' + encodeURIComponent(String(currentPreset))
        }).then(function (r) {
          if (!r.ok) throw new Error();
        }).catch(function () {
          st.textContent = 'Não foi possível definir o preset ativo no dispositivo.';
          st.className = 'err';
        });
      });

      document.getElementById('btn-save').addEventListener('click', function () {
        var btn = document.getElementById('btn-save');
        var st = document.getElementById('save-status');
        st.className = '';
        presets[currentPreset] = readPresetFromForm();
        var n = currentPreset;
        var body = 'preset=' + encodeURIComponent(String(n)) + '&json=' + encodeURIComponent(JSON.stringify(presets[n]));
        btn.disabled = true;
        st.textContent = 'Salvando…';
        fetch('/save', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: body })
          .then(function (r) { return r.json().catch(function () { return {}; }); })
          .then(function (j) {
            btn.disabled = false;
            if (j && j.ok) {
              st.textContent = 'Preset ' + n + ' salvo no dispositivo.';
              st.className = 'ok';
            } else {
              st.textContent = 'Falha ao salvar (verifique a rede ou o tamanho do preset).';
              st.className = 'err';
            }
          })
          .catch(function () {
            btn.disabled = false;
            st.textContent = 'Erro de rede ao salvar.';
            st.className = 'err';
          });
      });

      bindTabs();
      bindPressRadios();
      updateAllFeetUi();

      Promise.all([
        fetch('/presets').then(function (r) { return r.json(); }),
        fetch('/active').then(function (r) { return r.json(); }).catch(function () { return { active: 1 }; })
      ])
        .then(function (pair) {
          var all = pair[0];
          var act = pair[1] && pair[1].active ? parseInt(pair[1].active, 10) : 1;
          if (act >= 1 && act <= 10) {
            currentPreset = act;
            sel.value = String(act);
          }
          for (var k in all) {
            if (!Object.prototype.hasOwnProperty.call(all, k)) continue;
            var idx = parseInt(k, 10);
            if (idx >= 1 && idx <= 10 && all[k] && all[k].feet) presets[idx] = all[k];
          }
          applyPresetToForm(presets[currentPreset]);
        })
        .catch(function () {
          applyPresetToForm(presets[1]);
        });
    })();
  </script>
</body>
</html>
)rawliteral";

#endif
