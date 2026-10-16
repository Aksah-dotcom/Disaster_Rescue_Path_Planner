# Phase 1 — Core Algorithms & Data (C++)

The foundation, ported from the JS backend's `graph.js`, `algorithms/*.js`,
`journey.js`, and `data/*.js`. No server, no HTTP, no external
dependencies beyond a C++17 compiler.

## What's in here
- `src/graph.hpp` — the `Graph` class, with a real Observer pattern
  (callback list) replacing JS's `EventEmitter`
- `src/algorithms/` — BFS, DFS, Dijkstra (using `std::priority_queue`),
  K-shortest (Yen's algorithm), and `RoutingStrategy` — a genuine
  abstract base class with `virtual` functions, giving real inheritance
  and polymorphism (the original standalone C++ console file, from
  earlier in this project, was explicitly missing both)
- `src/journey.hpp` — `Journey`/`JourneyManager`, subscribing to the
  graph's block events for dynamic re-routing
- `src/data/` — the same 25-town, 35-road Uttarakhand dataset and the
  two historical disaster scenarios

## Build & run
```bash
g++ -std=c++17 -Isrc test.cpp -o test
./test
```

## Done when
- All four strategies find correct paths for DEH -> BAD
- K-shortest returns 3 distinct routes
- Blocking the road directly ahead of an active journey reroutes it
  from its actual current position, not the original start
- Connectivity check shows 24/25 (only Kedarnath excluded — correct)

## Handed off to Phase 2
Phase 2 wraps this exact code in an HTTP server — nothing here changes.
