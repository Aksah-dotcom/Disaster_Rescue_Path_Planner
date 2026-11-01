# Test Checklist — C++ Disaster Rescue Path Planner

Every item here was actually verified during development of the C++ port.

## Build
- [ ] `make` produces both `server` and `client` with no errors
- [ ] Compiles clean with `-Wall` (no warnings suppressed)

## Core algorithms (no server needed — see Phase 1 for isolated tests)
- [ ] All four strategies (bfs/dfs/dijkstra/kshortest) find correct
      paths for DEH -> BAD via a base-class `RoutingStrategy*` pointer
- [ ] K-shortest returns 3 genuinely distinct routes, zero leftover
      temp-blocked edges afterward
- [ ] Blocking a road ahead of an active Journey reroutes it from its
      actual current position (confirmed via direct test, not just API)

## HTTP API
- [ ] `GET /health` → `{"status":"ok"}`
- [ ] `POST /api/route` matches the JS backend's results exactly
      (same path, same cost, for the same input)
- [ ] `POST /api/route/k-shortest`, `/api/journey` + advance/delete,
      `GET /api/route/export` (produces a file `file` identifies as a
      real PDF), `GET /api/replay/scenarios` — all tested directly

## Frontend (served by the C++ server via static file mount)
- [ ] `GET /`, `/css/style.css`, `/js/app.js`, `/js/api.js`,
      `/js/offlineAlgorithms.js`, `/sw.js` all return 200 from the same
      C++ binary that serves the API — confirmed no separate server
      needed
- [ ] The map loads and a route can be planned end-to-end through the browser

## Known bug found and fixed during this port
`routeMetrics(...).items()` — iterating a view over a temporary `json`
object that gets destroyed before the loop body runs (a C++-specific
lifetime issue with no JS equivalent). Caused `/api/route` to hang
indefinitely. Fixed by binding to a named variable first. See the main
README's "A real bug this conversion caught" section.

## Deployment
- [ ] `docker compose up -d --build` builds and starts successfully
- [ ] `curl http://localhost:4000/health` works against the container
- [ ] Restarting the container preserves `data/state.json` (named volume)

## Honest known limitations (not fixed, flagged instead)
- No mutex around `Graph` — the traffic-simulator thread and HTTP
  request threads both mutate it with no locking. Untested under real
  concurrent load.
- PDF export is a minimal hand-written implementation (plain text, one
  page) — no external PDF library was available/installed.
