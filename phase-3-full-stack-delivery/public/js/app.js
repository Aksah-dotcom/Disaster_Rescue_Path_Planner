let graphData = { nodes: [], edges: [] };
let nodeById = {};
let algo = "dijkstra";
let lastResult = null;
let lastSyncedAt = null;
let isOffline = false;
let activeJourneyId = null;
let journeyPollTimer = null;
let replayActive = false;

const consoleEl = document.getElementById("console");
function logMsg(msg, tag, tagClass) {
  const d = document.createElement("div");
  const cls = tagClass ? ` ${tagClass}` : "";
  d.innerHTML = (tag ? `<span class="tag${cls}">[${tag}]</span> ` : "") + msg;
  consoleEl.appendChild(d);
  consoleEl.scrollTop = consoleEl.scrollHeight;
}
function setApiStatus(ok) {
  document.getElementById("statusDot").classList.toggle("ok", ok);
  document.getElementById("apiWarning").hidden = ok;
}
function updateSyncNote() {
  if (!lastSyncedAt) return;
  const secs = Math.max(0, Math.round((Date.now() - lastSyncedAt) / 1000));
  document.getElementById("syncNote").textContent = `synced ${secs}s ago`;
}
setInterval(updateSyncNote, 1000);

if ("serviceWorker" in navigator) {
  navigator.serviceWorker.register("/sw.js").catch(() => {});
}
function setOfflineBanner(offline) { isOffline = offline; document.getElementById("apiWarning").hidden = !offline; }
window.addEventListener("offline", () => setOfflineBanner(true));
window.addEventListener("online", () => { setOfflineBanner(false); pollTraffic(); });
function cacheGraphLocally(data) { try { localStorage.setItem("rescue-planner-graph-cache", JSON.stringify(data)); } catch (e) {} }
function loadCachedGraph() { try { const raw = localStorage.getItem("rescue-planner-graph-cache"); return raw ? JSON.parse(raw) : null; } catch (e) { return null; } }

document.querySelectorAll(".tab-btn").forEach((btn) => {
  btn.addEventListener("click", () => {
    document.querySelectorAll(".tab-btn").forEach((b) => b.classList.remove("active"));
    btn.classList.add("active");
    document.querySelectorAll(".tab-panel").forEach((p) => (p.hidden = true));
    document.getElementById(`tab-${btn.dataset.tab}`).hidden = false;
  });
});

const map = L.map("map", { zoomControl: true, attributionControl: true }).setView([30.0, 79.0], 8);
L.tileLayer("https://tile.openstreetmap.org/{z}/{x}/{y}.png", {
  maxZoom: 19,
  attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

const roadLayer = L.layerGroup().addTo(map);
const markerLayer = L.layerGroup().addTo(map);
const pathLayer = L.layerGroup().addTo(map);
const markers = {};
const TRAFFIC_COLOR = { low: "#34A853", medium: "#FBBC04", high: "#EA4335" };
const ALT_ROUTE_COLORS = ["#1A73E8", "#8E44AD", "#00897B"];

function pinIcon(color, scale) {
  scale = scale || 1;
  const w = 22 * scale, h = 30 * scale;
  return L.divIcon({
    className: "pin",
    html: `<svg width="${w}" height="${h}" viewBox="0 0 22 30"><path d="M11 0C4.9 0 0 4.9 0 11c0 8.2 11 19 11 19s11-10.8 11-19C22 4.9 17.1 0 11 0z" fill="${color}"/><circle cx="11" cy="11" r="4.2" fill="#fff"/></svg>`,
    iconSize: [w, h], iconAnchor: [w / 2, h], popupAnchor: [0, -h],
  });
}
const DOT_COLOR = "#5F6368", VISITED_COLOR = "#FBBC04", START_COLOR = "#34A853", END_COLOR = "#EA4335", PATH_COLOR = "#1A73E8", TEAM_COLOR = "#8E44AD";

function edgeLatLngs(e) {
  if (e.real && e.waypoints && e.waypoints.length) return e.waypoints;
  const A = nodeById[e.a], B = nodeById[e.b];
  const mid = (e.waypoints || []).map(([lat, lng]) => [lat, lng]);
  return [[A.lat, A.lng], ...mid, [B.lat, B.lng]];
}
function findEdge(g, a, b) { return g.edges.find((x) => (x.a === a && x.b === b) || (x.a === b && x.b === a)); }

function drawRoads() {
  roadLayer.clearLayers();
  graphData.edges.forEach((e) => {
    if (!nodeById[e.a] || !nodeById[e.b]) return;
    const latlngs = edgeLatLngs(e);
    let line;
    if (e.blocked) line = L.polyline(latlngs, { color: "#EA4335", weight: 5, dashArray: "7,7", opacity: 0.95 });
    else line = L.polyline(latlngs, { color: TRAFFIC_COLOR[e.trafficLevel] || TRAFFIC_COLOR.low, weight: 5, opacity: 0.85, className: `road-flow ${e.trafficLevel}` });
    const sourceTag = e.real ? "live route via OSRM" : "estimated route";
    line.bindTooltip(
      e.blocked ? `${e.road} (${e.a}–${e.b}) — BLOCKED${e.blockReason ? " · " + e.blockReason : ""}` :
      `${e.road} (${e.a}–${e.b}) — ${e.distanceKm} km · ${e.trafficLevel} traffic · ~${e.travelTimeMin ?? ""} min · ${sourceTag}`,
      { sticky: true }
    );
    if (!replayActive) line.on("click", () => toggleRoad(e));
    line.addTo(roadLayer);
  });
}
let weatherData = {};

function drawMarkers() {
  markerLayer.clearLayers();
  const showWeatherPins = document.getElementById("weatherToggle") ? document.getElementById("weatherToggle").checked : true;
  graphData.nodes.forEach((n) => {
    const w = weatherData[n.id];
    const weatherHtml = w ? `
      <div style="margin-top:8px;padding:7px 10px;background:#F8FAFC;border:1px solid #E2E8F0;border-radius:6px;font-size:11.5px;line-height:1.4;">
        <div style="display:flex;align-items:center;gap:6px;">
          <span style="font-size:16px;">${w.icon}</span>
          <b style="color:#1E293B;">${w.condition}</b>
          <span style="margin-left:auto;font-weight:bold;color:#2563EB;">${w.tempC}°C</span>
        </div>
        <div style="margin-top:3px;color:#64748B;font-size:10.5px;">
          Rain: <b>${w.rainMm} mm</b> · Precip: <b>${w.precipitationMm} mm</b>
        </div>
        ${w.rainMm >= 4.0 ? '<div style="margin-top:4px;color:#DC2626;font-weight:bold;font-size:10px;">⚠️ Heavy Rain — High Landslide Risk</div>' : ''}
      </div>` : `<div style="margin-top:6px;font-size:10.5px;color:#94A3B8;">Fetching live weather…</div>`;

    const m = L.marker([n.lat, n.lng], { icon: pinIcon(DOT_COLOR, 0.72) });
    m.bindPopup(`<b>${n.label}</b><br>${n.sub}<br><span style="color:#80868B">${n.id} · ${n.lat.toFixed(4)}, ${n.lng.toFixed(4)}</span>${weatherHtml}`);
    if (w) {
      if (showWeatherPins) {
        m.bindTooltip(`${w.icon} ${w.tempC}°C`, { permanent: true, direction: "top", offset: [0, -18], className: "weather-pill" });
      } else {
        m.bindTooltip(`${n.label} ${w.icon} ${w.tempC}°C`, { direction: "top", offset: [0, -18], opacity: 0.92 });
      }
    }
    m.addTo(markerLayer);
    markers[n.id] = m;
  });
}
const weatherToggleEl = document.getElementById("weatherToggle");
if (weatherToggleEl) weatherToggleEl.addEventListener("change", () => drawMarkers());
function setMarkerState(id, state) {
  const m = markers[id]; if (!m) return;
  if (state === "visited") m.setIcon(pinIcon(VISITED_COLOR, 0.85));
  else if (state === "path") m.setIcon(pinIcon(PATH_COLOR, 0.85));
  else if (state === "start") m.setIcon(pinIcon(START_COLOR, 1.15));
  else if (state === "end") m.setIcon(pinIcon(END_COLOR, 1.15));
  else if (state === "team") m.setIcon(pinIcon(TEAM_COLOR, 1.1));
  else m.setIcon(pinIcon(DOT_COLOR, 0.72));
}
function render() {
  drawRoads(); drawMarkers(); updateBlockedCount();
  if (lastResult && lastResult.found && !replayActive) reapplyPathHighlight();
  else if (!lastResult) pathLayer.clearLayers();
}
function reapplyPathHighlight() {
  pathLayer.clearLayers();
  if (!lastResult || !lastResult.path) return;
  const { path } = lastResult;
  for (let i = 0; i < path.length; i++) {
    setMarkerState(path[i], i === 0 ? "start" : i === path.length - 1 ? "end" : "path");
    if (i > 0) {
      const e = findEdge(graphData, path[i - 1], path[i]);
      if (e && !e.blocked) L.polyline(edgeLatLngs(e), { color: PATH_COLOR, weight: 6, opacity: 0.95, interactive: false }).addTo(pathLayer);
    }
  }
}
function updateBlockedCount() {
  const n = graphData.edges.filter((e) => e.blocked).length;
  document.getElementById("blockedCount").textContent = `${n} road${n !== 1 ? "s" : ""} blocked`;
}

async function toggleRoad(edge) {
  try {
    const resp = edge.blocked ? await Api.unblockRoad(edge.a, edge.b) : await Api.blockRoad(edge.a, edge.b, "reported blocked");
    graphData = resp.graph;
    cacheGraphLocally(graphData);
    nodeById = Object.fromEntries(graphData.nodes.map((n) => [n.id, n]));
    lastSyncedAt = Date.now();
    logMsg(edge.blocked ? `${edge.road} (${edge.a}–${edge.b}) cleared — reopened` : `${edge.road} (${edge.a}–${edge.b}) reported blocked`, "OPS");

    // If an active journey is running, refresh it
    if (activeJourneyId) {
      try { const j = await Api.getJourney(activeJourneyId); renderJourney(j); } catch (e) {}
    }

    // If an active route was planned, dynamically recompute to route around block or update
    if (lastResult && lastResult.start && lastResult.end && !replayActive) {
      await autoReroute();
    } else {
      render();
    }
  } catch (err) { logMsg(`Failed to update road: ${err.message}`, "ERROR", "error"); }
}

async function pollTraffic() {
  if (replayActive) return;
  try {
    const fresh = await Api.getGraph();
    const oldByKey = Object.fromEntries(graphData.edges.map((e) => [`${e.a}__${e.b}`, e]));
    let changes = 0;
    fresh.edges.forEach((e) => {
      const prev = oldByKey[`${e.a}__${e.b}`];
      if (prev && !prev.blocked && !e.blocked && prev.trafficLevel !== e.trafficLevel) {
        changes++;
        if (changes <= 2) logMsg(`Traffic on ${e.road} shifted to ${e.trafficLevel.toUpperCase()}`, "TRAFFIC", "traffic");
      }
      if (prev && !prev.real && e.real) logMsg(`Live route data arrived for ${e.road} — now ${e.distanceKm} km real distance`, "ROUTING");
    });
    graphData = fresh;
    cacheGraphLocally(graphData);
    nodeById = Object.fromEntries(graphData.nodes.map((n) => [n.id, n]));
    lastSyncedAt = Date.now();
    setApiStatus(true); setOfflineBanner(false);
    render();
  } catch (err) { setApiStatus(false); setOfflineBanner(true); }
}
setInterval(pollTraffic, 6000);

function populateSelects() {
  const startSel = document.getElementById("startSel");
  const endSel = document.getElementById("endSel");
  startSel.innerHTML = ""; endSel.innerHTML = "";
  const sorted = [...graphData.nodes].sort((a, b) => a.label.localeCompare(b.label));
  sorted.forEach((n) => {
    startSel.appendChild(new Option(`${n.label} (${n.id})`, n.id));
    endSel.appendChild(new Option(`${n.label} (${n.id})`, n.id));
  });
  startSel.value = "DEH"; endSel.value = "BAD";
}
document.getElementById("swapBtn").addEventListener("click", () => {
  const startSel = document.getElementById("startSel"), endSel = document.getElementById("endSel");
  const tmp = startSel.value; startSel.value = endSel.value; endSel.value = tmp;
});
document.querySelectorAll(".algo-btn").forEach((b) => {
  b.addEventListener("click", () => {
    document.querySelectorAll(".algo-btn").forEach((x) => x.classList.remove("active"));
    b.classList.add("active"); algo = b.dataset.a;
    document.getElementById("exportRow").hidden = true;
  });
});

const runBtn = document.getElementById("runBtn");
const resultEl = document.getElementById("result");
function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

async function autoReroute() {
  const start = document.getElementById("startSel").value;
  const end = document.getElementById("endSel").value;

  if (algo === "kshortest") {
    await runKShortest(start, end);
    return;
  }

  let res;
  try {
    if (isOffline) throw new Error("offline");
    res = await Api.findRoute(start, end, algo);
  } catch (err) {
    const cached = graphData || loadCachedGraph();
    res = OfflineRouting.computeRoute(cached, start, end, algo);
  }

  pathLayer.clearLayers();
  drawMarkers();

  if (!res.found) {
    lastResult = null;
    setMarkerState(start, "start");
    setMarkerState(end, "end");
    resultEl.innerHTML = `<div class="k" style="color:var(--alert);font-weight:600;">⚠️ Road blocked — no alternate route exists between ${start} and ${end}.</div>`;
    logMsg(`Road blocked — route between ${start} and ${end} is now cut off`, "OPS", "error");
    document.getElementById("exportRow").hidden = true;
    return;
  }

  for (let i = 0; i < res.path.length; i++) {
    setMarkerState(res.path[i], i === 0 ? "start" : i === res.path.length - 1 ? "end" : "path");
    if (i > 0) {
      const e = findEdge(graphData, res.path[i - 1], res.path[i]);
      if (e) L.polyline(edgeLatLngs(e), { color: PATH_COLOR, weight: 6, opacity: 0.95, interactive: false }).addTo(pathLayer);
    }
  }

  lastResult = res;
  const hops = res.path.length - 1;
  const label = { bfs: "BFS — fewest roads", dfs: "DFS — first available route", dijkstra: "Dijkstra — lowest total risk" }[algo] || algo;
  resultEl.innerHTML = `
    <div class="stat-row">
      <div class="stat eta"><div class="num">${res.etaMinutes}<span class="unit">min</span></div><div class="lbl">ETA</div></div>
      <div class="stat risk"><div class="num">${res.totalDistanceKm}<span class="unit">km</span></div><div class="lbl">Distance</div></div>
    </div>
    <div><span class="k">Algorithm:</span> ${label} <span style="color:#137333;font-size:11.5px;font-weight:600;">(dynamic reroute)</span></div>
    <div><span class="k">Route:</span> <span class="path">${res.path.join(" → ")}</span></div>
    <div class="via"><span class="k">Via:</span> ${(res.roads || []).join(", ")}</div>
    <div><span class="k">Roads used:</span> ${hops}</div>
    ${res.cost !== null ? `<div><span class="k">Total risk score:</span> ${res.cost}</div>` : ""}
  `;
  document.getElementById("exportRow").hidden = false;
  logMsg(`Auto-rerouted: ${res.path.join(" → ")} — ETA ${res.etaMinutes} min (${res.totalDistanceKm} km)`, "REROUTE", "traffic");
}

async function run() {
  const start = document.getElementById("startSel").value;
  const end = document.getElementById("endSel").value;
  runBtn.disabled = true; lastResult = null;
  document.getElementById("exportRow").hidden = true;
  pathLayer.clearLayers();
  render();
  logMsg(`Request: ${start} → ${end}`, algo.toUpperCase());

  if (algo === "kshortest") { await runKShortest(start, end); runBtn.disabled = false; return; }

  let res;
  try {
    if (isOffline) throw new Error("offline");
    res = await Api.findRoute(start, end, algo);
  } catch (err) {
    logMsg(`Live API unavailable — computing locally with cached data.`, "OFFLINE", "traffic");
    const cached = graphData || loadCachedGraph();
    res = OfflineRouting.computeRoute(cached, start, end, algo);
  }

  for (const id of res.order || []) { setMarkerState(id, "visited"); await sleep(90); }

  if (!res.found) {
    pathLayer.clearLayers();
    drawMarkers();
    setMarkerState(start, "start");
    setMarkerState(end, "end");
    resultEl.innerHTML = `<div class="k" style="color:var(--alert);font-weight:600;">⚠️ No route found — ${start} is cut off from ${end} given current blocked roads.</div>`;
    logMsg(`No path exists — all routes blocked`, "RESULT", "result");
    runBtn.disabled = false; return;
  }

  pathLayer.clearLayers();
  for (let i = 0; i < res.path.length; i++) {
    setMarkerState(res.path[i], i === 0 ? "start" : i === res.path.length - 1 ? "end" : "path");
    if (i > 0) {
      const e = findEdge(graphData, res.path[i - 1], res.path[i]);
      if (e) L.polyline(edgeLatLngs(e), { color: PATH_COLOR, weight: 6, opacity: 0.95, interactive: false }).addTo(pathLayer);
    }
    await sleep(100);
  }

  lastResult = res;
  const allLatLngs = pathLayer.getLayers().flatMap((l) => l.getLatLngs());
  if (allLatLngs.length) map.fitBounds(allLatLngs, { padding: [40, 40] });

  const hops = res.path.length - 1;
  const label = { bfs: "BFS — fewest roads", dfs: "DFS — first available route", dijkstra: "Dijkstra — lowest total risk" }[algo] || algo;
  resultEl.innerHTML = `
    <div class="stat-row">
      <div class="stat eta"><div class="num">${res.etaMinutes}<span class="unit">min</span></div><div class="lbl">ETA</div></div>
      <div class="stat risk"><div class="num">${res.totalDistanceKm}<span class="unit">km</span></div><div class="lbl">Distance</div></div>
    </div>
    <div><span class="k">Algorithm:</span> ${label}</div>
    <div><span class="k">Route:</span> <span class="path">${res.path.join(" → ")}</span></div>
    <div class="via"><span class="k">Via:</span> ${(res.roads || []).join(", ")}</div>
    <div><span class="k">Roads used:</span> ${hops}</div>
    ${res.cost !== null ? `<div><span class="k">Total risk score:</span> ${res.cost}</div>` : ""}
  `;
  document.getElementById("exportRow").hidden = false;
  logMsg(`Route found: ${res.path.join(" → ")} — ETA ${res.etaMinutes} min (${res.totalDistanceKm} km)`, "RESULT", "result");
  runBtn.disabled = false;
}
runBtn.addEventListener("click", run);

async function runKShortest(start, end) {
  let data;
  try {
    if (isOffline) throw new Error("offline");
    data = await Api.findKShortest(start, end, 3);
  } catch (err) {
    logMsg("K-shortest paths requires the live API (not available offline).", "ERROR", "error");
    resultEl.innerHTML = `<div class="k">K-shortest paths needs a live connection.</div>`;
    return;
  }
  pathLayer.clearLayers();
  data.routes.forEach((r, i) => {
    const color = ALT_ROUTE_COLORS[i % ALT_ROUTE_COLORS.length];
    for (let j = 0; j < r.path.length - 1; j++) {
      const e = findEdge(graphData, r.path[j], r.path[j + 1]);
      if (e) L.polyline(edgeLatLngs(e), { color, weight: i === 0 ? 6 : 4, opacity: i === 0 ? 0.95 : 0.65, dashArray: i === 0 ? null : "2,8", interactive: false }).addTo(pathLayer);
    }
  });
  setMarkerState(start, "start"); setMarkerState(end, "end");
  const allLatLngs = pathLayer.getLayers().flatMap((l) => l.getLatLngs());
  if (allLatLngs.length) map.fitBounds(allLatLngs, { padding: [40, 40] });
  resultEl.innerHTML = `
    <div class="k" style="margin-bottom:8px;">Top ${data.routes.length} distinct routes, ranked by risk score:</div>
    ${data.routes.map((r, i) => `
      <div class="kroute" style="border-left:3px solid ${ALT_ROUTE_COLORS[i % ALT_ROUTE_COLORS.length]}">
        <div><b>#${r.rank}</b> — ${r.cost} risk · ${r.etaMinutes} min · ${r.totalDistanceKm} km</div>
        <div class="via">${r.path.join(" → ")}</div>
      </div>`).join("")}
  `;
  lastResult = { path: data.routes[0].path, start, end, found: true };
  logMsg(`Found ${data.routes.length} distinct routes ${start} → ${end}`, "RESULT", "result");
}

document.getElementById("exportBtn").addEventListener("click", async () => {
  if (!lastResult || !lastResult.found) return;
  const exportBtn = document.getElementById("exportBtn");
  exportBtn.disabled = true;
  exportBtn.textContent = "⏳ Generating PDF…";
  logMsg("Capturing map and generating route PDF…", "EXPORT");

  try {
    // ── 1. Fit map to the route bounds before capture ──────────────
    const routeLatLngs = pathLayer.getLayers().flatMap((l) => l.getLatLngs());
    if (routeLatLngs.length) map.fitBounds(routeLatLngs, { padding: [50, 50] });
    // small delay for tiles to load after fit
    await sleep(600);

    // ── 2. Capture the map div ─────────────────────────────────────
    const mapEl = document.getElementById("map");
    const canvas = await html2canvas(mapEl, {
      useCORS: true,
      allowTaint: true,
      scale: 2,
      logging: false,
      backgroundColor: "#E5E3DF",
    });

    // ── 3. Build the PDF (A4 landscape for wider map) ──────────────
    const { jsPDF } = window.jspdf;
    const pdf = new jsPDF({ orientation: "landscape", unit: "mm", format: "a4" });
    const pageW = pdf.internal.pageSize.getWidth();   // ~297
    const pageH = pdf.internal.pageSize.getHeight();   // ~210
    const margin = 12;
    const usableW = pageW - margin * 2;

    // -- Header bar --
    pdf.setFillColor(26, 115, 232);
    pdf.rect(0, 0, pageW, 22, "F");
    pdf.setFont("helvetica", "bold");
    pdf.setFontSize(14);
    pdf.setTextColor(255, 255, 255);
    pdf.text("Uttarakhand Disaster Rescue — Route Sheet", margin, 14);
    pdf.setFontSize(8);
    pdf.setFont("helvetica", "normal");
    pdf.text(new Date().toLocaleString(), pageW - margin, 14, { align: "right" });

    // -- Route summary line --
    const startNode = nodeById[lastResult.start || lastResult.path[0]];
    const endNode = nodeById[lastResult.end || lastResult.path[lastResult.path.length - 1]];
    const startLabel = startNode ? startNode.label : lastResult.path[0];
    const endLabel = endNode ? endNode.label : lastResult.path[lastResult.path.length - 1];

    let yPos = 30;
    pdf.setTextColor(32, 33, 36);
    pdf.setFont("helvetica", "bold");
    pdf.setFontSize(13);
    pdf.text(`${startLabel}  →  ${endLabel}`, margin, yPos);

    // -- Algorithm & stats --
    yPos += 7;
    pdf.setFont("helvetica", "normal");
    pdf.setFontSize(9);
    const algoLabels = { bfs: "BFS (fewest roads)", dfs: "DFS (first found)", dijkstra: "Dijkstra (lowest risk)", kshortest: "K-Shortest Paths" };
    const stats = [];
    stats.push(`Algorithm: ${algoLabels[algo] || algo}`);
    if (lastResult.etaMinutes) stats.push(`ETA: ${lastResult.etaMinutes} min`);
    if (lastResult.totalDistanceKm) stats.push(`Distance: ${lastResult.totalDistanceKm} km`);
    if (lastResult.cost !== null && lastResult.cost !== undefined) stats.push(`Risk Score: ${lastResult.cost}`);
    stats.push(`Roads: ${lastResult.path.length - 1}`);
    pdf.setTextColor(95, 99, 104);
    pdf.text(stats.join("   •   "), margin, yPos);

    // -- Map image --
    yPos += 5;
    const imgData = canvas.toDataURL("image/png");
    const mapAspect = canvas.width / canvas.height;
    let mapW = usableW * 0.62;
    let mapH = mapW / mapAspect;
    const maxMapH = pageH - yPos - margin - 5;
    if (mapH > maxMapH) { mapH = maxMapH; mapW = mapH * mapAspect; }

    // border around map
    pdf.setDrawColor(224, 227, 231);
    pdf.setLineWidth(0.3);
    pdf.rect(margin - 0.5, yPos - 0.5, mapW + 1, mapH + 1, "S");
    pdf.addImage(imgData, "PNG", margin, yPos, mapW, mapH);

    // -- Route details panel (right of map) --
    const panelX = margin + mapW + 8;
    const panelW = usableW - mapW - 8;

    // Turn-by-turn directions
    let dirY = yPos;
    pdf.setFont("helvetica", "bold");
    pdf.setFontSize(10);
    pdf.setTextColor(26, 115, 232);
    pdf.text("Turn-by-Turn Route", panelX, dirY);
    dirY += 6;

    pdf.setFontSize(8);
    const path = lastResult.path;
    for (let i = 0; i < path.length; i++) {
      const node = nodeById[path[i]];
      const label = node ? node.label : path[i];
      const id = path[i];

      // Step marker
      if (i === 0) {
        pdf.setFillColor(52, 168, 83); // green start
        pdf.circle(panelX + 3, dirY - 1.2, 2.2, "F");
        pdf.setTextColor(255, 255, 255);
        pdf.setFont("helvetica", "bold");
        pdf.text("S", panelX + 1.8, dirY);
      } else if (i === path.length - 1) {
        pdf.setFillColor(234, 67, 53); // red end
        pdf.circle(panelX + 3, dirY - 1.2, 2.2, "F");
        pdf.setTextColor(255, 255, 255);
        pdf.setFont("helvetica", "bold");
        pdf.text("E", panelX + 1.8, dirY);
      } else {
        pdf.setFillColor(26, 115, 232);
        pdf.circle(panelX + 3, dirY - 1.2, 1.8, "F");
        pdf.setTextColor(255, 255, 255);
        pdf.setFont("helvetica", "bold");
        pdf.text(String(i), panelX + 1.8 + (i < 10 ? 0.5 : 0), dirY);
      }

      // Town name
      pdf.setTextColor(32, 33, 36);
      pdf.setFont("helvetica", "bold");
      pdf.setFontSize(8.5);
      pdf.text(label, panelX + 8, dirY);
      pdf.setFont("helvetica", "normal");
      pdf.setFontSize(7);
      pdf.setTextColor(128, 134, 139);
      pdf.text(`(${id})`, panelX + 8 + pdf.getTextWidth(label + " "), dirY);

      // Road connector to next
      if (i < path.length - 1) {
        const edge = findEdge(graphData, path[i], path[i + 1]);
        if (edge) {
          dirY += 4;
          // draw a thin connecting line
          pdf.setDrawColor(200, 200, 200);
          pdf.setLineWidth(0.2);
          pdf.line(panelX + 3, dirY - 3.5, panelX + 3, dirY + 0.5);
          // road info
          pdf.setFontSize(6.5);
          pdf.setTextColor(95, 99, 104);
          const trafficBadge = edge.trafficLevel === "high" ? "🔴" : edge.trafficLevel === "medium" ? "🟡" : "🟢";
          const roadInfo = `↓  ${edge.road}  •  ${edge.distanceKm} km  ${trafficBadge} ${edge.trafficLevel}`;
          pdf.text(roadInfo, panelX + 8, dirY);
          dirY += 5;
        } else {
          dirY += 5;
        }
      }

      // Check if we need a new page
      if (dirY > pageH - margin - 5 && i < path.length - 1) {
        pdf.addPage();
        dirY = margin + 5;
        pdf.setFont("helvetica", "bold");
        pdf.setFontSize(9);
        pdf.setTextColor(26, 115, 232);
        pdf.text("Route (continued)", panelX, dirY);
        dirY += 6;
      }
    }

    // -- Route path summary (bottom of right panel) --
    dirY += 4;
    if (dirY < pageH - margin - 15) {
      pdf.setDrawColor(224, 227, 231);
      pdf.line(panelX, dirY, panelX + panelW, dirY);
      dirY += 5;
      pdf.setFontSize(7);
      pdf.setTextColor(95, 99, 104);
      pdf.setFont("helvetica", "normal");
      const pathStr = path.join(" → ");
      const splitPath = pdf.splitTextToSize(pathStr, panelW);
      pdf.text(splitPath, panelX, dirY);
    }

    // -- Footer --
    pdf.setFontSize(6.5);
    pdf.setTextColor(128, 134, 139);
    pdf.text("Generated by Uttarakhand Disaster Rescue Path Planner (C++ Backend)", margin, pageH - 5);
    pdf.text("Map data © OpenStreetMap contributors © CARTO", pageW - margin, pageH - 5, { align: "right" });

    // ── 4. Save ────────────────────────────────────────────────────
    pdf.save(`route-${lastResult.path[0]}-to-${lastResult.path[lastResult.path.length - 1]}.pdf`);
    logMsg("Route PDF with map exported successfully.", "EXPORT");
  } catch (err) {
    logMsg(`PDF export failed: ${err.message}. Falling back to server export.`, "ERROR", "error");
    // fallback to server-side export
    const url = Api.exportUrl(lastResult.start || lastResult.path[0], lastResult.end || lastResult.path[lastResult.path.length - 1], algo === "kshortest" ? "dijkstra" : algo);
    window.open(url, "_blank");
  } finally {
    exportBtn.disabled = false;
    exportBtn.textContent = "⭳ Export PDF route sheet";
  }
});

const journeyStatusEl = document.getElementById("journeyStatus");
const advanceBtn = document.getElementById("advanceBtn");
document.getElementById("startJourneyBtn").addEventListener("click", async () => {
  if (!lastResult || !lastResult.found) {
    journeyStatusEl.innerHTML = `<div class="k">Plan a route on the "Plan Route" tab first.</div>`;
    return;
  }
  try {
    const journey = await Api.startJourney(lastResult.start, lastResult.end, algo === "kshortest" ? "dijkstra" : algo);
    activeJourneyId = journey.id;
    renderJourney(journey);
    advanceBtn.hidden = false;
    logMsg(`Journey started: ${journey.path.join(" → ")}`, "JOURNEY");
    if (journeyPollTimer) clearInterval(journeyPollTimer);
    journeyPollTimer = setInterval(async () => {
      if (!activeJourneyId) return;
      try { const j = await Api.getJourney(activeJourneyId); renderJourney(j); } catch (e) {}
    }, 4000);
  } catch (err) { journeyStatusEl.innerHTML = `<div class="k">${err.message}</div>`; }
});
advanceBtn.addEventListener("click", async () => {
  if (!activeJourneyId) return;
  const j = await Api.advanceJourney(activeJourneyId);
  renderJourney(j);
  logMsg(`Team advanced to ${j.currentNode}`, "JOURNEY");
});
function renderJourney(j) {
  pathLayer.clearLayers();
  const teamIdx = j.path.indexOf(j.currentNode);
  for (let i = 0; i < j.path.length; i++) {
    if (j.path[i] === j.currentNode) {
      setMarkerState(j.path[i], "team");
    } else {
      setMarkerState(j.path[i], i === 0 ? "start" : i === j.path.length - 1 ? "end" : "path");
    }
    if (i > 0) {
      const e = findEdge(graphData, j.path[i - 1], j.path[i]);
      if (e) {
        const isRemaining = i > teamIdx;
        L.polyline(edgeLatLngs(e), {
          color: isRemaining ? PATH_COLOR : "#9AA0A6",
          weight: 6,
          opacity: isRemaining ? 0.95 : 0.45,
          interactive: false
        }).addTo(pathLayer);
      }
    }
  }
  const rerouteNote = j.rerouteHistory.length
    ? `<div class="k" style="margin-top:8px;color:#B31412;">⚠ Rerouted ${j.rerouteHistory.length}x — most recent: ${j.rerouteHistory[j.rerouteHistory.length - 1].reason}</div>` : "";
  journeyStatusEl.innerHTML = `
    <div><span class="k">Team is at:</span> <b>${nodeById[j.currentNode]?.label || j.currentNode}</b></div>
    <div class="via"><span class="k">Remaining:</span> ${j.remainingPath.join(" → ")}</div>
    ${j.isComplete ? `<div class="k" style="color:#137333;margin-top:6px;">✓ Journey complete</div>` : ""}
    ${rerouteNote}
  `;
  if (j.isComplete && journeyPollTimer) { clearInterval(journeyPollTimer); journeyPollTimer = null; advanceBtn.hidden = true; }
}

async function loadScenarios() {
  try {
    const scenarios = await Api.getScenarios();
    const sel = document.getElementById("scenarioSel");
    sel.innerHTML = "";
    scenarios.forEach((s) => sel.appendChild(new Option(s.label, s.key)));
  } catch (err) { logMsg("Could not load historical scenarios (needs live API).", "ERROR", "error"); }
}
document.getElementById("playReplayBtn").addEventListener("click", async () => {
  const key = document.getElementById("scenarioSel").value;
  const replayLog = document.getElementById("replayLog");
  replayLog.innerHTML = "";
  let scenario;
  try { scenario = await Api.getScenario(key); } catch (err) { replayLog.innerHTML = `<div class="k">${err.message}</div>`; return; }

  replayActive = true;
  const snapshot = JSON.parse(JSON.stringify(graphData));
  lastResult = null;
  logMsg(`Replaying: ${scenario.label} (does not affect the live shared map)`, "REPLAY", "traffic");

  for (const ev of scenario.events) {
    const line = document.createElement("div");
    line.innerHTML = `<b>Hour ${ev.hour}:</b> ${ev.note || ev.type}`;
    replayLog.appendChild(line); replayLog.scrollTop = replayLog.scrollHeight;

    if (ev.type === "block") { const e = findEdge(graphData, ev.a, ev.b); if (e) { e.blocked = true; e.blockReason = ev.reason; } }
    else if (ev.type === "unblock") { const e = findEdge(graphData, ev.a, ev.b); if (e) { e.blocked = false; e.blockReason = null; } }
    else if (ev.type === "trafficSpike") { (ev.roads || []).forEach(([a, b]) => { const e = findEdge(graphData, a, b); if (e) e.trafficLevel = ev.level; }); }
    render();
    await sleep(1000);
  }
  logMsg(`Replay finished. Reverting to live map.`, "REPLAY", "traffic");
  await sleep(500);
  graphData = snapshot;
  replayActive = false;
  render();
});

async function init() {
  try {
    graphData = await Api.getGraph();
    cacheGraphLocally(graphData);
    nodeById = Object.fromEntries(graphData.nodes.map((n) => [n.id, n]));
    lastSyncedAt = Date.now();
    setApiStatus(true);
    populateSelects();
    render();
    map.fitBounds(graphData.nodes.map((n) => [n.lat, n.lng]), { padding: [30, 30] });
    logMsg("Uttarakhand road network loaded from C++ backend.", "INIT");
    loadScenarios();

    // Fetch weather asynchronously in background without blocking map render
    Api.getWeather().then((w) => {
      if (w && Object.keys(w).length) {
        weatherData = w;
        drawMarkers();
        logMsg("Live weather conditions updated across all Uttarakhand stations.", "WEATHER");
      }
    }).catch(() => {});
    setInterval(async () => {
      try {
        const w = await Api.getWeather();
        if (w && Object.keys(w).length) { weatherData = w; drawMarkers(); }
      } catch (e) {}
    }, 180000);
  } catch (err) {
    const cached = loadCachedGraph();
    if (cached) {
      graphData = cached;
      nodeById = Object.fromEntries(graphData.nodes.map((n) => [n.id, n]));
      populateSelects(); render(); setOfflineBanner(true);
      logMsg("Could not reach the API — loaded last cached map for offline use.", "OFFLINE", "traffic");
    } else {
      setApiStatus(false);
      logMsg(`Could not reach API and no cached data available: ${err.message}`, "ERROR", "error");
    }
  }
}
init();
