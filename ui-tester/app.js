const fallbackFirmwareUi = {
  pageCount: 4,
  pages: [
    { key: "logo", enumName: "Logo", value: 0, title: "Welcome", subtitle: "Research dashboard landing" },
    { key: "dashboard", enumName: "Dashboard", value: 1, title: "Main Dashboard", subtitle: "Tap left = previous, right = next" },
    { key: "trends", enumName: "Trends", value: 2, title: "Live Trends", subtitle: "Temp + humidity + distance chart" },
    { key: "logging", enumName: "Logging", value: 3, title: "SD Logging", subtitle: "Sensor-only build: CSV preview/status" },
  ],
  palette: {
    screenBg: "#081222",
    panelBg: "#0e1b32",
    panelBorder: "#244a7a",
    headerBg: "#0c1e38",
    headerBorder: "#3876b6",
    footerBg: "#0a172c",
    textStrong: "#f7fafd",
    textMuted: "#97c1df",
    textSoft: "#7da4c2",
    grid: "#224067",
    chartBg: "#091527",
    liveBg: "#1b8254",
    liveText: "#e9fcef",
    offlineBg: "#88262b",
    offlineText: "#ffe5e5",
  },
  metrics: {
    temperature: { title: "Temperature", unit: "deg C", min: 10, max: 40, decimals: 1, accent: "#ff9f43" },
    humidity: { title: "Humidity", unit: "% RH", min: 20, max: 90, decimals: 1, accent: "#00c8ff" },
    distance: { title: "Distance", unit: "mm", min: 0, max: 200, decimals: 0, accent: "#7effa3" },
  },
  copy: {
    brand: "Capture Healing",
    logoTitleLine1: "Capture",
    logoTitleLine2: "Healing",
    logoSubtitleLine1: "Cold Atmospheric",
    logoSubtitleLine2: "Plasma Research",
    trendsHeading: "Temperature + Humidity + Distance",
    loggingHeading: "SD Logging Status",
    footerOnline: "Tap left for previous | right for next",
  },
};

const firmwareUi =
  window.FIRMWARE_UI && Array.isArray(window.FIRMWARE_UI.pages)
    ? window.FIRMWARE_UI
    : fallbackFirmwareUi;

const pages = (firmwareUi.pages || fallbackFirmwareUi.pages).slice().sort((a, b) => a.value - b.value);
const pageOrder = pages.map((page) => page.key);
const pageByKey = Object.fromEntries(pages.map((page) => [page.key, page]));
const metrics = {
  ...fallbackFirmwareUi.metrics,
  ...(firmwareUi.metrics || {}),
};
const palette = {
  ...fallbackFirmwareUi.palette,
  ...(firmwareUi.palette || {}),
};
const copy = {
  ...fallbackFirmwareUi.copy,
  ...(firmwareUi.copy || {}),
};

const SENSOR_SAMPLE_INTERVAL_MS = 1000;
const LOG_WRITE_INTERVAL_MS = 3000;
const metricOrder = ["temperature", "humidity", "distance"];

const dom = {
  screen: document.getElementById("screen"),
  brandTitle: document.getElementById("brandTitle"),
  uptime: document.getElementById("uptime"),
  footer: document.getElementById("footer"),
  pageTitle: document.getElementById("pageTitle"),
  pageTag: document.getElementById("pageTag"),
  prevPageBtn: document.getElementById("prevPageBtn"),
  nextPageBtn: document.getElementById("nextPageBtn"),
  logoTitleLine1: document.getElementById("logoTitleLine1"),
  logoTitleLine2: document.getElementById("logoTitleLine2"),
  logoSubtitleLine1: document.getElementById("logoSubtitleLine1"),
  logoSubtitleLine2: document.getElementById("logoSubtitleLine2"),
  trendsHeading: document.getElementById("trendsHeading"),
  loggingHeading: document.getElementById("loggingHeading"),
  pages: {
    logo: document.getElementById("page-logo"),
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
  trendDistanceValue: document.getElementById("trendDistanceValue"),
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
    rows: [],
    lastWriteMs: null,
    nextWriteDueMs: performance.now() + LOG_WRITE_INTERVAL_MS,
  },
};

function applyFirmwareConfig() {
  const root = document.documentElement;
  const paletteVars = {
    screenBg: "--screen-a",
    panelBg: "--panel-bg",
    panelBorder: "--panel-border",
    headerBg: "--header-bg",
    headerBorder: "--header-border",
    footerBg: "--footer-bg",
    textStrong: "--value-text",
    textMuted: "--muted-text",
    textSoft: "--soft-text",
    grid: "--grid-color",
    chartBg: "--chart-bg",
    liveBg: "--live-bg",
    liveText: "--live-text",
    offlineBg: "--offline-bg",
    offlineText: "--offline-text",
  };

  for (const [key, variable] of Object.entries(paletteVars)) {
    if (palette[key]) root.style.setProperty(variable, palette[key]);
  }

  for (const metric of metricOrder) {
    const cfg = metrics[metric];
    if (!cfg) continue;
    const accentVar =
      metric === "temperature" ? "--temp-accent" : metric === "humidity" ? "--humidity-accent" : "--distance-accent";
    root.style.setProperty(accentVar, cfg.accent);

    const card = document.querySelector(`[data-metric="${metric}"]`);
    if (card) {
      const title = card.querySelector("h2");
      const unit = card.querySelector(".metric-unit");
      if (title) title.textContent = cfg.title;
      if (unit) unit.textContent = cfg.unit;
    }
  }

  dom.brandTitle.textContent = copy.brand;
  dom.logoTitleLine1.textContent = copy.logoTitleLine1;
  dom.logoTitleLine2.textContent = copy.logoTitleLine2;
  dom.logoSubtitleLine1.textContent = copy.logoSubtitleLine1;
  dom.logoSubtitleLine2.textContent = copy.logoSubtitleLine2;
  dom.trendsHeading.textContent = copy.trendsHeading;
  dom.loggingHeading.textContent = copy.loggingHeading;

  dom.controls.tempRange.min = metrics.temperature.min;
  dom.controls.tempRange.max = metrics.temperature.max;
  dom.controls.humidityRange.min = metrics.humidity.min;
  dom.controls.humidityRange.max = metrics.humidity.max;
  dom.controls.distanceRange.min = metrics.distance.min;
  dom.controls.distanceRange.max = metrics.distance.max;
}

function ensureGeneratedPages() {
  for (const page of pages) {
    if (dom.pages[page.key]) continue;

    const section = document.createElement("section");
    section.className = "panel generic-page tft-page hidden";
    section.id = `page-${page.key}`;

    const title = document.createElement("h2");
    title.textContent = page.title;
    const subtitle = document.createElement("p");
    subtitle.textContent = page.subtitle || "Firmware page detected by sync generator.";
    section.append(title, subtitle);

    dom.screen.insertBefore(section, dom.footer);
    dom.pages[page.key] = section;
  }
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function metricConfig(metric) {
  return metrics[metric] || fallbackFirmwareUi.metrics[metric];
}

function formatValue(metric, value) {
  return Number(value).toFixed(metricConfig(metric).decimals);
}

function formatMetricDisplay(metric, online) {
  if (!online) return "--";
  const cfg = metricConfig(metric);
  return `${formatValue(metric, state.values[metric])} ${cfg.unit}`;
}

function normalize(metric, value) {
  const cfg = metricConfig(metric);
  return clamp((value - cfg.min) / (cfg.max - cfg.min), 0, 1);
}

function randomStep(value, metric) {
  const cfg = metricConfig(metric);
  const amplitude = metric === "distance" ? 4.5 : 0.35;
  return clamp(value + (Math.random() * 2 - 1) * amplitude, cfg.min, cfg.max);
}

function buildLogFilename(nowMs) {
  const day = String(Math.floor(nowMs / 86400000) + 1).padStart(8, "0");
  return `env_${day}.csv`;
}

function pushHistory() {
  const maxPoints = 180;
  for (const metric of metricOrder) {
    state.history[metric].push(state.values[metric]);
    if (state.history[metric].length > maxPoints) state.history[metric].shift();
  }
}

function createCsvRow(nowMs) {
  return {
    timeSec: String(Math.floor(nowMs / 1000)),
    temperature: state.online.shtc3 ? formatValue("temperature", state.values.temperature) : "--",
    humidity: state.online.shtc3 ? formatValue("humidity", state.values.humidity) : "--",
    distance: state.online.distance ? formatValue("distance", state.values.distance) : "--",
  };
}

function tickData() {
  if (state.animate) {
    for (const metric of metricOrder) {
      state.values[metric] = randomStep(state.values[metric], metric);
    }

    dom.controls.tempRange.value = String(state.values.temperature);
    dom.controls.humidityRange.value = String(state.values.humidity);
    dom.controls.distanceRange.value = String(state.values.distance);
  }

  pushHistory();
  const nowMs = performance.now();
  state.lastSampleMs = nowMs;

  if (state.online.sd && nowMs >= state.log.nextWriteDueMs) {
    writeLogNow(nowMs);
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
  const width = canvas.width;
  const height = canvas.height;
  const history = state.history[metric];

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = palette.chartBg;
  ctx.fillRect(0, 0, width, height);
  ctx.strokeStyle = palette.grid;
  ctx.strokeRect(0.5, 0.5, width - 1, height - 1);

  if (!online || history.length < 2) {
    ctx.fillStyle = palette.textSoft;
    ctx.font = "10px Bahnschrift, Trebuchet MS, Segoe UI, sans-serif";
    ctx.fillText("Collecting...", 6, height / 2 + 4);
    return;
  }

  ctx.beginPath();
  for (let index = 0; index < history.length; index += 1) {
    const x = (index / (history.length - 1)) * (width - 1);
    const y = height - 1 - normalize(metric, history[index]) * (height - 1);
    if (index === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.strokeStyle = metricConfig(metric).accent;
  ctx.lineWidth = 2;
  ctx.stroke();
}

function drawTrendsPage() {
  const canvas = dom.trendCanvas;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = palette.chartBg;
  ctx.fillRect(0, 0, width, height);
  ctx.strokeStyle = palette.panelBorder;
  ctx.strokeRect(0.5, 0.5, width - 1, height - 1);

  for (let index = 1; index <= 4; index += 1) {
    const y = (height / 5) * index;
    ctx.strokeStyle = "rgba(71, 117, 163, 0.3)";
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  const plotMetric = (metric, online) => {
    const history = state.history[metric];
    if (!online || history.length < 2) return;
    ctx.beginPath();
    for (let index = 0; index < history.length; index += 1) {
      const x = (index / (history.length - 1)) * (width - 1);
      const y = height - 1 - normalize(metric, history[index]) * (height - 1);
      if (index === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = metricConfig(metric).accent;
    ctx.lineWidth = 2.2;
    ctx.stroke();
  };

  plotMetric("temperature", state.online.shtc3);
  plotMetric("humidity", state.online.shtc3);
  plotMetric("distance", state.online.distance);

  dom.trendTempValue.textContent = formatMetricDisplay("temperature", state.online.shtc3);
  dom.trendHumidityValue.textContent = formatMetricDisplay("humidity", state.online.shtc3);
  dom.trendDistanceValue.textContent = formatMetricDisplay("distance", state.online.distance);
  dom.trendSampleCount.textContent = String(state.history.temperature.length);
}

function renderLoggingPage(nowMs) {
  dom.logFileName.textContent = buildLogFilename(nowMs);

  if (state.online.sd) {
    dom.sdStatusBadge.textContent = "CSV READY";
    dom.sdStatusBadge.classList.add("badge-live");
    dom.sdStatusBadge.classList.remove("badge-offline");
  } else {
    dom.sdStatusBadge.textContent = "CSV PAUSED";
    dom.sdStatusBadge.classList.add("badge-offline");
    dom.sdStatusBadge.classList.remove("badge-live");
  }

  const recentRows = state.log.rows.slice(-5).reverse();
  const rows = [];
  for (let index = 0; index < 5; index += 1) {
    const row = recentRows[index];
    if (row) {
      rows.push(
        `<tr><td>${row.timeSec}</td><td>${row.temperature}</td><td>${row.humidity}</td><td>${row.distance}</td></tr>`
      );
    } else {
      rows.push("<tr><td>-</td><td>-</td><td>-</td><td>-</td></tr>");
    }
  }
  dom.csvTableBody.innerHTML = rows.join("");
}

function renderFooter(nowMs) {
  const ageSec = Math.floor((nowMs - state.lastSampleMs) / 1000);
  if (state.online.shtc3 && state.online.distance) {
    dom.footer.textContent = copy.footerOnline;
  } else if (!state.online.shtc3 && !state.online.distance) {
    dom.footer.textContent = "All sensors offline | check wiring";
  } else if (!state.online.shtc3) {
    dom.footer.textContent = `SHTC3 offline | sample age: ${ageSec}s`;
  } else {
    dom.footer.textContent = `VL6180X offline | sample age: ${ageSec}s`;
  }
}

function setPage(index) {
  const pageCount = pageOrder.length || 1;
  state.pageIndex = ((index % pageCount) + pageCount) % pageCount;
  const page = pageOrder[state.pageIndex];

  for (const [key, element] of Object.entries(dom.pages)) {
    if (element) element.classList.toggle("hidden", key !== page);
  }

  const pageMeta = pageByKey[page] || pages[state.pageIndex] || fallbackFirmwareUi.pages[0];
  dom.screen.dataset.page = page;
  dom.pageTitle.textContent = pageMeta.title;
  dom.pageTag.textContent = `P${state.pageIndex + 1}/${pageCount}`;
}

function renderDashboardPage() {
  const metricOnline = {
    temperature: state.online.shtc3,
    humidity: state.online.shtc3,
    distance: state.online.distance,
  };

  for (const metric of metricOrder) {
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
  renderLoggingPage(nowMs);
  renderFooter(nowMs);
}

function bindControls() {
  dom.nextPageBtn.addEventListener("click", () => {
    setPage(state.pageIndex + 1);
    render();
  });

  dom.prevPageBtn.addEventListener("click", () => {
    setPage(state.pageIndex - 1);
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
  applyFirmwareConfig();
  ensureGeneratedPages();
  bindControls();
  for (let index = 0; index < 30; index += 1) pushHistory();
  writeLogNow(performance.now());
  setPage(0);
  render();

  window.setInterval(() => {
    tickData();
    render();
  }, SENSOR_SAMPLE_INTERVAL_MS);
}

init();
