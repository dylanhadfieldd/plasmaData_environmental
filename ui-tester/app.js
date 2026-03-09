const ranges = {
  temperature: { min: 10, max: 40, decimals: 1 },
  humidity: { min: 20, max: 90, decimals: 1 },
  distance: { min: 0, max: 200, decimals: 0 },
};

const pageOrder = ["dashboard", "trends", "logging"];
const SENSOR_SAMPLE_INTERVAL_MS = 1000;
const LOG_WRITE_INTERVAL_MS = 3000;
const pageTitles = {
  dashboard: "Main Dashboard",
  trends: "Live Trends: Temp + Humidity",
  logging: "Live Data Logging (CSV to SD)",
};

const dom = {
  uptime: document.getElementById("uptime"),
  footer: document.getElementById("footer"),
  pageTitle: document.getElementById("pageTitle"),
  nextPageBtn: document.getElementById("nextPageBtn"),
  pages: {
    dashboard: document.getElementById("page-dashboard"),
    trends: document.getElementById("page-trends"),
    logging: document.getElementById("page-logging"),
  },
  values: {
    temperature: document.querySelector('[data-value="temperature"]'),
    humidity: document.querySelector('[data-value="humidity"]'),
    distance: document.querySelector('[data-value="distance"]'),
  },
  bars: {
    temperature: document.querySelector('[data-bar="temperature"]'),
    humidity: document.querySelector('[data-bar="humidity"]'),
    distance: document.querySelector('[data-bar="distance"]'),
  },
  badges: {
    temperature: document.querySelector('[data-badge="temperature"]'),
    humidity: document.querySelector('[data-badge="humidity"]'),
    distance: document.querySelector('[data-badge="distance"]'),
  },
  sparks: {
    temperature: document.querySelector('[data-spark="temperature"]'),
    humidity: document.querySelector('[data-spark="humidity"]'),
    distance: document.querySelector('[data-spark="distance"]'),
  },
  trendCanvas: document.getElementById("trendsCanvas"),
  trendTempValue: document.getElementById("trendTempValue"),
  trendHumidityValue: document.getElementById("trendHumidityValue"),
  trendSampleCount: document.getElementById("trendSampleCount"),
  sdStatusBadge: document.getElementById("sdStatusBadge"),
  logFileName: document.getElementById("logFileName"),
  csvTableBody: document.getElementById("csvTableBody"),
  controls: {
    tempRange: document.getElementById("tempRange"),
    humidityRange: document.getElementById("humidityRange"),
    distanceRange: document.getElementById("distanceRange"),
    shtc3Online: document.getElementById("shtc3Online"),
    distanceOnline: document.getElementById("distanceOnline"),
    animate: document.getElementById("animate"),
    sdInstalled: document.getElementById("sdInstalled"),
  },
};

const state = {
  startedMs: performance.now(),
  lastSampleMs: performance.now(),
  pageIndex: 0,
  values: {
    temperature: Number(dom.controls.tempRange.value),
    humidity: Number(dom.controls.humidityRange.value),
    distance: Number(dom.controls.distanceRange.value),
  },
  online: {
    shtc3: true,
    distance: true,
    sd: true,
  },
  animate: true,
  history: {
    temperature: [],
    humidity: [],
    distance: [],
  },
  log: {
    filename: buildLogFilename(),
    rows: [],
    lastWriteMs: null,
    nextWriteDueMs: performance.now() + LOG_WRITE_INTERVAL_MS,
  },
};

const colors = {
  temperature: "#ff9f43",
  humidity: "#00c8ff",
  distance: "#7effa3",
};

function buildLogFilename() {
  const now = new Date();
  const yyyy = now.getFullYear();
  const mm = String(now.getMonth() + 1).padStart(2, "0");
  const dd = String(now.getDate()).padStart(2, "0");
  return `env_${yyyy}${mm}${dd}.csv`;
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function formatValue(metric, value) {
  return value.toFixed(ranges[metric].decimals);
}

function normalize(metric, value) {
  const cfg = ranges[metric];
  return clamp((value - cfg.min) / (cfg.max - cfg.min), 0, 1);
}

function randomStep(value, metric) {
  const cfg = ranges[metric];
  const amplitude = metric === "distance" ? 4.5 : 0.35;
  return clamp(value + (Math.random() * 2 - 1) * amplitude, cfg.min, cfg.max);
}

function pushHistory() {
  const maxPoints = 180;
  for (const metric of Object.keys(state.history)) {
    state.history[metric].push(state.values[metric]);
    if (state.history[metric].length > maxPoints) state.history[metric].shift();
  }
}

function createCsvRow(nowMs) {
  return {
    timeSec: String(Math.floor(nowMs / 1000)),
    temperature: state.values.temperature.toFixed(2),
    humidity: state.values.humidity.toFixed(2),
    distance: state.values.distance.toFixed(0),
  };
}

function tickData() {
  if (state.animate) {
    state.values.temperature = randomStep(state.values.temperature, "temperature");
    state.values.humidity = randomStep(state.values.humidity, "humidity");
    state.values.distance = randomStep(state.values.distance, "distance");

    dom.controls.tempRange.value = String(state.values.temperature);
    dom.controls.humidityRange.value = String(state.values.humidity);
    dom.controls.distanceRange.value = String(state.values.distance);
  }

  pushHistory();
  const nowMs = performance.now();
  state.lastSampleMs = nowMs;

  if (state.online.sd && nowMs >= state.log.nextWriteDueMs) {
    state.log.rows.push(createCsvRow(nowMs));
    if (state.log.rows.length > 300) state.log.rows.shift();
    state.log.lastWriteMs = nowMs;
    state.log.nextWriteDueMs = nowMs + LOG_WRITE_INTERVAL_MS;
  }
}

function writeLogNow(nowMs) {
  if (!state.online.sd) return;
  state.log.rows.push(createCsvRow(nowMs));
  if (state.log.rows.length > 300) state.log.rows.shift();
  state.log.lastWriteMs = nowMs;
  state.log.nextWriteDueMs = nowMs + LOG_WRITE_INTERVAL_MS;
}

function applyBadge(metric, online) {
  const badge = dom.badges[metric];
  if (!badge) return;
  badge.textContent = online ? "LIVE" : "OFFLINE";
  badge.classList.toggle("badge-live", online);
  badge.classList.toggle("badge-offline", !online);
}

function drawSmallSparkline(metric, online) {
  const canvas = dom.sparks[metric];
  if (!canvas) return;

  const ctx = canvas.getContext("2d");
  const w = canvas.width;
  const h = canvas.height;
  const history = state.history[metric];

  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = "#091527";
  ctx.fillRect(0, 0, w, h);
  ctx.strokeStyle = "#1f3e62";
  ctx.strokeRect(0.5, 0.5, w - 1, h - 1);

  if (!online || history.length < 2) {
    ctx.fillStyle = "#7da4c2";
    ctx.font = "10px Bahnschrift, Trebuchet MS, Segoe UI, sans-serif";
    ctx.fillText("Collecting...", 6, h / 2 + 4);
    return;
  }

  ctx.beginPath();
  for (let i = 0; i < history.length; i += 1) {
    const x = (i / (history.length - 1)) * (w - 1);
    const y = h - 1 - normalize(metric, history[i]) * (h - 1);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.strokeStyle = colors[metric];
  ctx.lineWidth = 2;
  ctx.stroke();
}

function drawTrendsPage() {
  const canvas = dom.trendCanvas;
  const ctx = canvas.getContext("2d");
  const w = canvas.width;
  const h = canvas.height;

  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = "#081425";
  ctx.fillRect(0, 0, w, h);
  ctx.strokeStyle = "#254970";
  ctx.strokeRect(0.5, 0.5, w - 1, h - 1);

  for (let i = 1; i <= 4; i += 1) {
    const gy = (h / 5) * i;
    ctx.strokeStyle = "rgba(71, 117, 163, 0.3)";
    ctx.beginPath();
    ctx.moveTo(0, gy);
    ctx.lineTo(w, gy);
    ctx.stroke();
  }

  const plotMetric = (metric, online) => {
    const history = state.history[metric];
    if (!online || history.length < 2) return;
    ctx.beginPath();
    for (let i = 0; i < history.length; i += 1) {
      const x = (i / (history.length - 1)) * (w - 1);
      const y = h - 1 - normalize(metric, history[i]) * (h - 1);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = colors[metric];
    ctx.lineWidth = 2.2;
    ctx.stroke();
  };

  plotMetric("temperature", state.online.shtc3);
  plotMetric("humidity", state.online.shtc3);

  dom.trendTempValue.textContent = state.online.shtc3
    ? `${formatValue("temperature", state.values.temperature)} deg C`
    : "--";
  dom.trendHumidityValue.textContent = state.online.shtc3
    ? `${formatValue("humidity", state.values.humidity)} % RH`
    : "--";
  dom.trendSampleCount.textContent = String(state.history.temperature.length);
}

function renderLoggingPage() {
  dom.logFileName.textContent = state.log.filename;

  if (state.online.sd) {
    dom.sdStatusBadge.textContent = "SD READY";
    dom.sdStatusBadge.classList.add("badge-live");
    dom.sdStatusBadge.classList.remove("badge-offline");
  } else {
    dom.sdStatusBadge.textContent = "SD MISSING";
    dom.sdStatusBadge.classList.add("badge-offline");
    dom.sdStatusBadge.classList.remove("badge-live");
  }

  const lastRows = state.log.rows.slice(-14);
  if (lastRows.length === 0) {
    dom.csvTableBody.innerHTML =
      '<tr><td colspan="4">Waiting for log samples... (writes every 3s)</td></tr>';
  } else {
    dom.csvTableBody.innerHTML = lastRows
      .map(
        (row) =>
          `<tr><td>${row.timeSec}</td><td>${row.temperature}</td><td>${row.humidity}</td><td>${row.distance}</td></tr>`
      )
      .join("");
  }
}

function renderFooter(nowMs) {
  const ageSec = Math.floor((nowMs - state.lastSampleMs) / 1000);
  if (state.online.shtc3 && state.online.distance && state.online.sd) {
    dom.footer.textContent = `Sensors healthy | SD write active | sample age: ${ageSec}s`;
  } else if (!state.online.sd) {
    dom.footer.textContent = `SD card missing | data not being saved | sample age: ${ageSec}s`;
  } else if (!state.online.shtc3 && !state.online.distance) {
    dom.footer.textContent = "All sensors offline | check wiring and power";
  } else if (!state.online.shtc3) {
    dom.footer.textContent = `SHTC3 offline | sample age: ${ageSec}s`;
  } else {
    dom.footer.textContent = `VL6180X offline | sample age: ${ageSec}s`;
  }
}

function setPage(index) {
  state.pageIndex = index % pageOrder.length;
  const page = pageOrder[state.pageIndex];

  for (const key of Object.keys(dom.pages)) {
    dom.pages[key].classList.toggle("hidden", key !== page);
  }
  dom.pageTitle.textContent = pageTitles[page];
}

function renderDashboardPage() {
  const metricOnline = {
    temperature: state.online.shtc3,
    humidity: state.online.shtc3,
    distance: state.online.distance,
  };

  for (const metric of Object.keys(dom.values)) {
    const online = metricOnline[metric];
    applyBadge(metric, online);

    if (online) {
      dom.values[metric].textContent = formatValue(metric, state.values[metric]);
      dom.bars[metric].style.width = `${normalize(metric, state.values[metric]) * 100}%`;
    } else {
      dom.values[metric].textContent = "--";
      dom.bars[metric].style.width = "0%";
    }

    drawSmallSparkline(metric, online);
  }
}

function render() {
  const nowMs = performance.now();
  const upSec = Math.floor((nowMs - state.startedMs) / 1000);
  const hh = String(Math.floor(upSec / 3600)).padStart(2, "0");
  const mm = String(Math.floor((upSec % 3600) / 60)).padStart(2, "0");
  const ss = String(upSec % 60).padStart(2, "0");
  dom.uptime.textContent = `UP ${hh}:${mm}:${ss}`;

  renderDashboardPage();
  drawTrendsPage();
  renderLoggingPage();
  renderFooter(nowMs);
}

function bindControls() {
  dom.nextPageBtn.addEventListener("click", () => {
    setPage(state.pageIndex + 1);
    render();
  });

  dom.controls.tempRange.addEventListener("input", (event) => {
    state.values.temperature = Number(event.target.value);
    pushHistory();
    state.lastSampleMs = performance.now();
    render();
  });

  dom.controls.humidityRange.addEventListener("input", (event) => {
    state.values.humidity = Number(event.target.value);
    pushHistory();
    state.lastSampleMs = performance.now();
    render();
  });

  dom.controls.distanceRange.addEventListener("input", (event) => {
    state.values.distance = Number(event.target.value);
    pushHistory();
    state.lastSampleMs = performance.now();
    render();
  });

  dom.controls.shtc3Online.addEventListener("change", (event) => {
    state.online.shtc3 = event.target.checked;
    render();
  });

  dom.controls.distanceOnline.addEventListener("change", (event) => {
    state.online.distance = event.target.checked;
    render();
  });

  dom.controls.sdInstalled.addEventListener("change", (event) => {
    state.online.sd = event.target.checked;
    render();
  });

  dom.controls.animate.addEventListener("change", (event) => {
    state.animate = event.target.checked;
  });
}

function init() {
  bindControls();
  for (let i = 0; i < 30; i += 1) pushHistory();
  writeLogNow(performance.now());
  setPage(0);
  render();

  window.setInterval(() => {
    tickData();
    render();
  }, SENSOR_SAMPLE_INTERVAL_MS);
}

init();
