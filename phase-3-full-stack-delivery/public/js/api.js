/**
 * api.js - thin wrapper around the backend REST API.
 * Uses relative paths because the frontend is served by the same server
 * process as the API (the C++ httplib server, via set_mount_point) —
 * no CORS, no base URL to configure.
 */
const Api = {
  async getGraph() {
    const res = await fetch(`/api/graph`);
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to load graph");
    return res.json();
  },
  async getWeather() {
    try {
      const res = await fetch(`/api/weather`);
      if (!res.ok) return {};
      return await res.json();
    } catch { return {}; }
  },
  async blockRoad(a, b, reason) {
    const res = await fetch(`/api/roads/block`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ a, b, reason }),
    });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to block road");
    return res.json();
  },
  async unblockRoad(a, b) {
    const res = await fetch(`/api/roads/unblock`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ a, b }),
    });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to unblock road");
    return res.json();
  },
  async findRoute(start, end, algorithm) {
    const res = await fetch(`/api/route`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ start, end, algorithm }),
    });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to compute route");
    return res.json();
  },
  async findKShortest(start, end, k) {
    const res = await fetch(`/api/route/k-shortest`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ start, end, k }),
    });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to compute k-shortest routes");
    return res.json();
  },
  async startJourney(start, destination, algorithm) {
    const res = await fetch(`/api/journey`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ start, destination, algorithm }),
    });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to start journey");
    return res.json();
  },
  async getJourney(id) {
    const res = await fetch(`/api/journey/${id}`);
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to load journey");
    return res.json();
  },
  async advanceJourney(id) {
    const res = await fetch(`/api/journey/${id}/advance`, { method: "POST" });
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to advance journey");
    return res.json();
  },
  async getScenarios() {
    const res = await fetch(`/api/replay/scenarios`);
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to load scenarios");
    return res.json();
  },
  async getScenario(key) {
    const res = await fetch(`/api/replay/${key}`);
    if (!res.ok) throw new Error((await safeJson(res)).error || "Failed to load scenario");
    return res.json();
  },
  exportUrl(start, end, algorithm) {
    return `/api/route/export?start=${start}&end=${end}&algorithm=${algorithm}`;
  },
};
async function safeJson(res) {
  try { return await res.json(); } catch { return {}; }
}
