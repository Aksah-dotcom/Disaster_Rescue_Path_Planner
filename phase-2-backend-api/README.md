# Phase 2 — Backend API (C++)

Wraps Phase 1's graph and algorithms in a real HTTP server (cpp-httplib).
No frontend yet — `index.html` doesn't exist in this phase at all,
matching how the original JS version's Phase 2 also had no real UI.

## What's new since Phase 1
- `server.cpp` — cpp-httplib HTTP server exposing the full REST API
- `routes.hpp` — all endpoints: `/api/route`, `/api/route/k-shortest`,
  `/api/roads/block|unblock`, `/api/journey` (+advance/delete),
  `/api/route/export` (PDF), `/api/replay/*`
- `persistence.hpp` — JSON state saved/restored via nlohmann/json
- `trafficSimulator.hpp` — background `std::thread`, same role as JS's `setInterval`
- `osrmClient.hpp` / `weatherClient.hpp` — live external API calls via libcurl,
  with the same graceful-fallback contract as the JS versions
- `client.cpp` — a console client for testing/using the API without a browser

## Build & run
```bash
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev
make
./server
```

## Test it
```bash
curl http://localhost:4000/health
curl -X POST http://localhost:4000/api/route \
  -H "Content-Type: application/json" \
  -d '{"start":"DEH","end":"BAD","algorithm":"dijkstra"}'
```
Or run `./client` in a second terminal for an interactive menu.

## Done when
- `/health` returns `{"status":"ok"}`
- Every endpoint from the README above responds correctly
- `curl .../api/route/export?...` produces a file that `file` identifies
  as a real PDF
- Blocking a road, restarting the server, and checking `/api/graph`
  shows the block survived
