/**
 * offlineAlgorithms.js
 * Client-side BFS/DFS/Dijkstra fallback for offline mode - mirrors the
 * backend algorithms (now in C++) exactly. Used only when a fetch to the
 * live API actually fails.
 */
const OfflineRouting = (() => {
  function neighbors(graphData, id) {
    return graphData.edges
      .filter((e) => !e.blocked && (e.a === id || e.b === id))
      .map((e) => ({ to: e.a === id ? e.b : e.a, w: e.weight }));
  }
  function buildPath(parent, start, end, visited) {
    if (!visited.has(end) && end !== start) return [];
    const path = [end];
    let cur = end;
    while (cur !== start) {
      cur = parent.get(cur);
      if (cur === undefined) return [];
      path.push(cur);
    }
    return path.reverse();
  }
  function bfs(graphData, start, end) {
    const parent = new Map();
    const visited = new Set([start]);
    const order = [];
    const q = [start];
    while (q.length) {
      const u = q.shift();
      order.push(u);
      if (u === end) break;
      for (const { to } of neighbors(graphData, u)) {
        if (visited.has(to)) continue;
        visited.add(to);
        parent.set(to, u);
        q.push(to);
      }
    }
    return { path: buildPath(parent, start, end, visited), order, cost: null };
  }
  function dfs(graphData, start, end) {
    const visited = new Set();
    const order = [];
    const parent = new Map();
    let found = false;
    (function walk(u) {
      if (found) return;
      visited.add(u);
      order.push(u);
      if (u === end) { found = true; return; }
      for (const { to } of neighbors(graphData, u)) {
        if (visited.has(to) || found) continue;
        parent.set(to, u);
        walk(to);
      }
    })(start);
    return { path: buildPath(parent, start, end, visited), order, cost: null };
  }
  function dijkstra(graphData, start, end) {
    const dist = new Map(), parent = new Map(), visited = new Set(), order = [];
    graphData.nodes.forEach((n) => dist.set(n.id, Infinity));
    dist.set(start, 0);
    const pq = [[0, start]];
    while (pq.length) {
      pq.sort((a, b) => a[0] - b[0]);
      const [d, u] = pq.shift();
      if (visited.has(u)) continue;
      visited.add(u);
      order.push(u);
      if (u === end) break;
      for (const { to, w } of neighbors(graphData, u)) {
        const nd = d + w;
        if (nd < dist.get(to)) {
          dist.set(to, nd);
          parent.set(to, u);
          pq.push([nd, to]);
        }
      }
    }
    const fd = dist.get(end);
    const path = fd < Infinity ? buildPath(parent, start, end, new Set([...visited, end])) : [];
    return { path, order, cost: fd === Infinity ? null : fd };
  }
  function travelTimeMinutes(distanceKm, trafficLevel) {
    const BASE = 42, MULT = { low: 1.0, medium: 0.55, high: 0.28 };
    return (distanceKm / (BASE * (MULT[trafficLevel] ?? 1))) * 60;
  }
  function computeRoute(graphData, start, end, algorithm) {
    const fn = algorithm === "bfs" ? bfs : algorithm === "dfs" ? dfs : dijkstra;
    const result = fn(graphData, start, end);
    let totalDistanceKm = 0, etaMinutes = 0;
    const roads = [];
    for (let i = 0; i < result.path.length - 1; i++) {
      const e = graphData.edges.find(
        (x) => (x.a === result.path[i] && x.b === result.path[i + 1]) || (x.a === result.path[i + 1] && x.b === result.path[i])
      );
      if (e) {
        totalDistanceKm += e.distanceKm;
        etaMinutes += travelTimeMinutes(e.distanceKm, e.trafficLevel);
        roads.push(e.road);
      }
    }
    return {
      start, end, algorithm, found: result.path.length > 0, ...result,
      roads: [...new Set(roads)],
      totalDistanceKm: Math.round(totalDistanceKm * 10) / 10,
      etaMinutes: Math.round(etaMinutes * 10) / 10,
    };
  }
  return { computeRoute };
})();
