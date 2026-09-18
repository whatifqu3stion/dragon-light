#pragma once

#include <Arduino.h>

const char kControlPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Dragon Light</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui, sans-serif; }
    body { margin: 0; background: #100d18; color: #f7f0ff; }
    main { max-width: 34rem; margin: auto; padding: 1.25rem; }
    h1 { margin: .25rem 0; font-size: 1.8rem; }
    .sub { color: #b9abc8; margin: 0 0 1.25rem; }
    .card { background: #211a2d; border: 1px solid #3b304b; border-radius: 1rem;
            padding: 1rem; margin: .85rem 0; }
    .row { display: flex; align-items: center; gap: .75rem; }
    .row input { flex: 1; accent-color: #ff9c45; }
    output { min-width: 3ch; font-size: 1.3rem; font-variant-numeric: tabular-nums; }
    .buttons { display: grid; grid-template-columns: 1fr 1fr; gap: .65rem; }
    button { border: 0; border-radius: .7rem; padding: .8rem; font: inherit;
             font-weight: 650; color: #18111f; background: #ffad64; }
    button.secondary { color: #f7f0ff; background: #453757; }
    button:active { transform: translateY(1px); }
    dl { display: grid; grid-template-columns: auto 1fr; gap: .45rem .85rem; margin: 0; }
    dt { color: #b9abc8; } dd { margin: 0; text-align: right; }
    #message { min-height: 1.3em; color: #ffcf9f; }
  </style>
</head>
<body>
<main>
  <h1>Dragon Light</h1>
  <p class="sub">Local display controls</p>

  <section class="card">
    <h2>Brightness</h2>
    <div class="row">
      <input id="brightness" type="range" min="5" max="128" value="64">
      <output id="brightnessValue">64</output>
    </div>
    <button id="saveBrightness">Save brightness</button>
  </section>

  <section class="card">
    <h2>Display</h2>
    <div class="buttons">
      <button data-mode="auto">Auto</button>
      <button class="secondary" data-mode="off">Off</button>
      <button class="secondary" data-mode="spin">Spin preview</button>
      <button class="secondary" data-mode="celebrate">Celebrate</button>
    </div>
  </section>

  <section class="card">
    <h2>Status</h2>
    <dl>
      <dt>Mode</dt><dd id="mode">—</dd>
      <dt>Rotation</dt><dd id="rotation">—</dd>
      <dt>Source</dt><dd id="source">—</dd>
      <dt>Time</dt><dd id="time">—</dd>
      <dt>Wi-Fi</dt><dd id="wifi">—</dd>
    </dl>
  </section>
  <p id="message" role="status"></p>
</main>
<script>
const slider = document.querySelector('#brightness');
const value = document.querySelector('#brightnessValue');
const message = document.querySelector('#message');
slider.addEventListener('input', () => value.textContent = slider.value);

async function request(path, values) {
  const response = await fetch(path, {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body: new URLSearchParams(values)
  });
  if (!response.ok) throw new Error(await response.text() || 'Request failed');
  return response.json();
}

function showStatus(status) {
  slider.value = status.brightness;
  value.textContent = status.brightness;
  document.querySelector('#mode').textContent = status.mode;
  document.querySelector('#rotation').textContent = status.rotation;
  document.querySelector('#source').textContent = status.source;
  document.querySelector('#time').textContent = status.time;
  document.querySelector('#wifi').textContent = status.ip;
}

async function refresh() {
  try {
    const response = await fetch('/api/status', {cache: 'no-store'});
    if (!response.ok) throw new Error('Status unavailable');
    showStatus(await response.json());
  } catch (error) {
    message.textContent = error.message;
  }
}

document.querySelector('#saveBrightness').addEventListener('click', async () => {
  try {
    showStatus(await request('/api/brightness', {value: slider.value}));
    message.textContent = 'Brightness saved.';
  } catch (error) { message.textContent = error.message; }
});

document.querySelectorAll('[data-mode]').forEach(button => {
  button.addEventListener('click', async () => {
    try {
      showStatus(await request('/api/mode', {value: button.dataset.mode}));
      message.textContent = `Mode set to ${button.textContent}.`;
    } catch (error) { message.textContent = error.message; }
  });
});

refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTML";
