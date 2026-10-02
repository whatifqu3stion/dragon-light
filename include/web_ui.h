#pragma once

#include <Arduino.h>

const char kControlPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta name="theme-color" content="#191816">
  <title>Dragon Light</title>
  <style>
    :root {
      color-scheme: dark;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      --bg: #191816; --panel: #23211e; --line: #49443c;
      --text: #f1ece2; --muted: #bcb3a6; --accent: #e3b878;
      --green: #b7ceb5; --error: #f0ada0;
    }
    * { box-sizing: border-box; }
    body { margin: 0; background: var(--bg); color: var(--text); font-size: 16px; line-height: 1.5; }
    ::selection { background: var(--accent); color: var(--bg); }
    main { width: min(100%, 760px); margin: auto; padding: 56px 32px 28px; }
    header { display: flex; align-items: center; justify-content: space-between; gap: 24px; margin-bottom: 32px; }
    .identity { display: flex; align-items: center; gap: 16px; }
    .mark { width: 38px; height: 44px; flex: none; color: var(--accent); }
    h1 { font-size: 30px; font-weight: 600; letter-spacing: -.025em; line-height: 1.2; margin: 0; }
    .sub { color: var(--muted); font-size: 14px; margin: 6px 0 0; }
    .connection { display: flex; align-items: center; gap: 8px; color: var(--muted); font-size: 14px; white-space: nowrap; }
    .connection::before { content: ""; width: 7px; height: 7px; border-radius: 50%; background: var(--muted); }
    .connection[data-state="online"] { color: var(--green); }
    .connection[data-state="online"]::before { background: var(--green); }
    .connection[data-state="offline"] { color: var(--error); }
    .connection[data-state="offline"]::before { background: var(--error); }
    .controls { background: var(--panel); border: 1px solid var(--line); border-radius: 14px; }
    .control-section { padding: 26px 28px 28px; }
    .control-section + .control-section { border-top: 1px solid var(--line); }
    h2 { margin: 0; font-size: 18px; line-height: 1.4; font-weight: 600; letter-spacing: -.01em; }
    .section-head { display: flex; align-items: center; justify-content: space-between; gap: 16px; }
    .section-head p { color: var(--muted); margin: 0; font-size: 14px; }
    .level { display: flex; align-items: baseline; gap: 8px; }
    output { font-size: 24px; font-weight: 500; font-variant-numeric: tabular-nums; }
    .level span { color: var(--muted); font-size: 14px; }
    .brightness-row { display: flex; align-items: center; gap: 28px; margin-top: 18px; }
    .range-control { flex: 1; min-width: 0; }
    input[type="range"] { display: block; width: 100%; height: 44px; margin: 0; padding: 0; cursor: pointer; appearance: none; background: transparent; accent-color: var(--accent); }
    input[type="range"]::-webkit-slider-runnable-track { height: 4px; border-radius: 2px; background: linear-gradient(to right, var(--accent) var(--level, 48%), #625a4e var(--level, 48%)); }
    input[type="range"]::-webkit-slider-thumb { appearance: none; width: 22px; height: 22px; margin-top: -9px; border-radius: 50%; background: var(--accent); border: 3px solid var(--panel); }
    input[type="range"]::-moz-range-track { height: 4px; border-radius: 2px; background: #625a4e; }
    input[type="range"]::-moz-range-progress { height: 4px; background: var(--accent); }
    input[type="range"]::-moz-range-thumb { width: 16px; height: 16px; border-radius: 50%; background: var(--accent); border: 3px solid var(--panel); }
    .range-limits { display: flex; justify-content: space-between; color: var(--muted); font-size: 13px; font-variant-numeric: tabular-nums; }
    button { min-height: 44px; padding: 11px 18px; border: 1px solid var(--line); border-radius: 8px; font: inherit; font-size: 14px; font-weight: 600; cursor: pointer; color: var(--text); background: transparent; transition: background-color 160ms ease-out, border-color 160ms ease-out; }
    button:hover { background: #302c26; border-color: #827564; }
    button:active { background: #393229; }
    button.primary { background: var(--accent); border-color: var(--accent); color: #272016; }
    button.primary:hover { background: #f0c88d; border-color: #f0c88d; }
    button:disabled, input:disabled { opacity: .45; cursor: not-allowed; }
    button:disabled:hover { background: transparent; border-color: var(--line); }
    button.primary:disabled:hover { background: var(--accent); border-color: var(--accent); }
    :focus-visible { outline: 2px solid var(--accent); outline-offset: 4px; }
    .buttons { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 10px; margin-top: 20px; }
    .mode-button { position: relative; display: flex; flex-direction: column; align-items: flex-start; gap: 12px; padding: 16px 14px; text-align: left; }
    .mode-button svg { width: 22px; height: 22px; flex-shrink: 0; color: var(--muted); }
    .mode-button span { display: block; }
    .mode-button[aria-pressed="true"] { background: #392f23; border-color: var(--accent); color: var(--accent); }
    .mode-button[aria-pressed="true"] svg { color: var(--accent); }
    .mode-button[aria-pressed="true"]::after { content: ""; position: absolute; top: 17px; right: 13px; width: 5px; height: 5px; border-radius: 50%; background: var(--accent); }
    .status-section { margin-top: 32px; }
    .refresh-button { display: inline-flex; align-items: center; gap: 8px; color: var(--muted); border-color: transparent; padding: 10px 8px; }
    .refresh-button svg { width: 16px; height: 16px; }
    dl { margin: 10px 0 0; }
    .status-row { display: grid; grid-template-columns: 112px minmax(0, 1fr); gap: 20px; padding: 11px 0; border-bottom: 1px solid #39352f; }
    dt { color: var(--muted); font-size: 14px; }
    dd { margin: 0; text-align: right; overflow-wrap: anywhere; font-size: 14px; font-variant-numeric: tabular-nums; }
    #message { min-height: 24px; margin: 18px 0 0; color: var(--muted); font-size: 14px; }
    #message[data-state="success"] { color: var(--green); }
    #message[data-state="error"] { color: var(--error); }
    .sr-only { position: absolute; width: 1px; height: 1px; padding: 0; margin: -1px; overflow: hidden; clip: rect(0,0,0,0); white-space: nowrap; border: 0; }
    @media (max-width: 560px) {
      main { padding: 28px 20px 20px; }
      header { align-items: flex-start; flex-direction: column; gap: 16px; margin-bottom: 24px; }
      h1 { font-size: 28px; }
      .connection { margin-left: 54px; }
      .control-section { padding: 22px 20px; }
      .brightness-row { flex-direction: column; align-items: stretch; gap: 18px; margin-top: 12px; }
      .buttons { grid-template-columns: repeat(2, minmax(0, 1fr)); }
      .mode-button { flex-direction: column; align-items: flex-start; gap: 10px; min-height: 88px; }
      .mode-button[aria-pressed="true"]::after { top: 10px; right: 10px; }
      .status-section { margin-top: 24px; }
    }
    @media (prefers-reduced-motion: reduce) { button { transition: none; } }
  </style>
</head>
<body>
<main>
  <header>
    <div class="identity">
      <svg class="mark" viewBox="0 0 38 44" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" aria-hidden="true"><path d="M8 4h16 M5 8v12 M5 25v11 M8 40h16 M28 7l5 8v14l-5 8 M10 22h13"/></svg>
      <div><h1>Dragon Light</h1><p class="sub">Local display controls</p></div>
    </div>
    <div id="connection" class="connection" data-state="connecting">Connecting…</div>
  </header>

  <div class="controls">
    <section class="control-section" aria-labelledby="brightness-heading">
      <div class="section-head">
        <h2 id="brightness-heading"><label for="brightness">Brightness</label></h2>
        <div class="level"><output id="brightnessValue" for="brightness" aria-label="Brightness level">64</output><span>/ 128</span></div>
      </div>
      <div class="brightness-row">
        <div class="range-control">
          <input id="brightness" type="range" min="5" max="128" value="64" disabled>
          <div class="range-limits" aria-hidden="true"><span>5 · Low</span><span>128 · High</span></div>
        </div>
        <button class="primary" id="saveBrightness" disabled>Save brightness</button>
      </div>
    </section>

    <section class="control-section" aria-labelledby="display-heading">
      <div class="section-head"><h2 id="display-heading">Display</h2><p>Choose a mode</p></div>
      <div class="buttons" role="group" aria-label="Display mode">
        <button class="mode-button" data-mode="auto" aria-pressed="false" disabled><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="4" y="5" width="16" height="15" rx="2"/><path d="M8 3v4 M16 3v4 M4 10h16 M8 14h2 M14 14h2 M8 17h2"/></svg><span>Auto</span></button>
        <button class="mode-button" data-mode="off" aria-pressed="false" disabled><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" aria-hidden="true"><path d="M12 3v8 M7 6a8 8 0 1 0 10 0"/></svg><span>Off</span></button>
        <button class="mode-button" data-mode="spin" aria-pressed="false" disabled><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M19 9a7.5 7.5 0 0 0-12-3 M5 15a7.5 7.5 0 0 0 12 3 M19 4v5h-5 M5 20v-5h5"/></svg><span>Spin preview</span></button>
        <button class="mode-button" data-mode="celebrate" aria-pressed="false" disabled><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m12 3 2.5 6.5L21 12l-6.5 2.5L12 21l-2.5-6.5L3 12l6.5-2.5Z"/></svg><span>Celebrate</span></button>
      </div>
    </section>
  </div>

  <section class="status-section" aria-labelledby="status-heading">
    <div class="section-head">
      <h2 id="status-heading">Device status</h2>
      <button id="refreshStatus" class="refresh-button"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M20 10a8 8 0 1 0-1 7 M20 4v6h-6"/></svg>Refresh<span class="sr-only"> status</span></button>
    </div>
    <dl>
      <div class="status-row"><dt>Mode</dt><dd id="mode">—</dd></div>
      <div class="status-row"><dt>Rotation</dt><dd id="rotation">—</dd></div>
      <div class="status-row"><dt>Source</dt><dd id="source">—</dd></div>
      <div class="status-row"><dt>Time</dt><dd id="time">—</dd></div>
      <div class="status-row"><dt>Wi-Fi</dt><dd id="wifi">—</dd></div>
    </dl>
  </section>
  <p id="message" role="status" aria-atomic="true">Checking device status…</p>
</main>
<script>
const slider = document.querySelector('#brightness');
const value = document.querySelector('#brightnessValue');
const message = document.querySelector('#message');
const connection = document.querySelector('#connection');
const saveButton = document.querySelector('#saveBrightness');
const refreshButton = document.querySelector('#refreshStatus');
const modeButtons = [...document.querySelectorAll('[data-mode]')];
const modeNames = {auto: 'Auto', off: 'Off', 'spin preview': 'Spin preview', 'celebration preview': 'Celebrate'};
const modeValues = {'spin preview': 'spin', 'celebration preview': 'celebrate'};
let dirty = false;
let busy = false;
let refreshing = false;
let connected = false;
let hasStatus = false;
let deviceBrightness;

function updateLevel() {
  value.textContent = slider.value;
  slider.style.setProperty('--level', ((slider.value - slider.min) / (slider.max - slider.min) * 100) + '%');
}
function setMessage(text, state = '') {
  message.textContent = text;
  message.dataset.state = state;
}
function updateControls() {
  slider.disabled = busy || !connected;
  saveButton.disabled = busy || refreshing || !connected || !dirty;
  modeButtons.forEach(button => button.disabled = busy || refreshing || !connected);
  refreshButton.disabled = busy || refreshing;
  document.querySelector('.controls').setAttribute('aria-busy', String(busy));
}
slider.addEventListener('input', () => {
  dirty = Number(slider.value) !== deviceBrightness;
  updateLevel();
  updateControls();
  setMessage(dirty ? 'Unsaved brightness. Save to apply.' : 'Brightness matches the device.');
});

async function fetchDevice(path, options = {}) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 10000);
  try {
    const response = await fetch(path, {...options, signal: controller.signal});
    if (!response.ok) throw new Error(await response.text() || 'Device request failed.');
    return await response.json();
  } catch (error) {
    if (error.name === 'AbortError' || error instanceof TypeError) {
      connected = false;
      connection.dataset.state = 'offline';
      connection.textContent = 'Device unreachable';
      throw new Error('Cannot reach the display. Check its Wi-Fi, then refresh status.' + (hasStatus ? ' Showing the last received status.' : ''));
    }
    throw error;
  } finally { clearTimeout(timeout); }
}
function showStatus(status) {
  deviceBrightness = Number(status.brightness);
  if (dirty && Number(slider.value) === deviceBrightness) dirty = false;
  if (!dirty) { slider.value = status.brightness; updateLevel(); }
  document.querySelector('#mode').textContent = modeNames[status.mode] || status.mode;
  document.querySelector('#rotation').textContent = status.rotation;
  document.querySelector('#source').textContent = status.source;
  document.querySelector('#time').textContent = status.time;
  document.querySelector('#wifi').textContent = status.ip;
  modeButtons.forEach(button => button.setAttribute('aria-pressed', String(button.dataset.mode === (modeValues[status.mode] || status.mode))));
  connected = true;
  hasStatus = true;
  connection.dataset.state = 'online';
  connection.textContent = 'Device connected';
  updateControls();
}
async function refresh() {
  if (busy || refreshing) return;
  const wasConnected = connected;
  refreshing = true;
  refreshButton.setAttribute('aria-busy', 'true');
  updateControls();
  try {
    showStatus(await fetchDevice('/api/status', {cache: 'no-store'}));
    if (!wasConnected) setMessage(dirty ? 'Connection restored. Brightness is still unsaved.' : 'Device status is current.');
  } catch (error) {
    connected = false;
    connection.dataset.state = 'offline';
    connection.textContent = 'Status unavailable';
    setMessage(error.message + (error.message.includes('refresh') ? '' : ' Try refreshing status.'), 'error');
  } finally {
    refreshing = false;
    refreshButton.removeAttribute('aria-busy');
    updateControls();
  }
}
async function apply(path, settings, successText) {
  if (busy || refreshing || !connected) return;
  busy = true;
  updateControls();
  setMessage('Applying to the display…');
  try {
    showStatus(await fetchDevice(path, {
      method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body: new URLSearchParams(settings)
    }));
    setMessage(successText + (dirty ? ' Brightness is still unsaved.' : ''), 'success');
  } catch (error) { setMessage(error.message + (connected ? ' Try again.' : ''), 'error'); }
  finally { busy = false; updateControls(); }
}
saveButton.addEventListener('click', () => apply('/api/brightness', {value: slider.value}, 'Brightness saved.'));
modeButtons.forEach(button => button.addEventListener('click', () => apply('/api/mode', {value: button.dataset.mode}, 'Mode set to ' + button.textContent.trim() + '.')));
refreshButton.addEventListener('click', refresh);
updateLevel();
refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTML";
