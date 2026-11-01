# Disaster Rescue Path Planner — C++ Backend

A complete C++ port of the Node.js/Express backend. Every piece of
*server-side* logic — the graph, all four routing algorithms, dynamic
re-routing, live external API integration, persistence, and the HTTP API
itself — has a real, compiled, tested C++ equivalent here. The one thing
that could **not** be converted, and why, is explained below rather than
glossed over.

## Build & run

```bash
# Ubuntu/Debian dependencies:
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev

make
./server        # starts the API on http://localhost:4000
./client        # in a second terminal — the console client (see below)
```

Environment variables (same names as the JS version):
`PORT`, `ENABLE_LIVE_ROUTING=false`, `ENABLE_WEATHER=false`,
`OSRM_BASE_URL`, `WEATHER_BASE_URL`, `DATA_DIR`.

## What could NOT be converted, and why

**The browser frontend (`public/index.html`, `style.css`, `api.js`,
`app.js`, `offlineAlgorithms.js`, `sw.js`) cannot become C++.** This
isn't a limitation of effort — it's a limitation of what a web browser
executes. Browsers run JavaScript (and HTML/CSS for structure/style);
they have no C++ runtime. There is no compiler flag or clever trick that
makes a browser execute a `.cpp` file. Converting "the part that draws
the map and lets you click things" to C++ would mean either:

- Compiling C++ to WebAssembly and still writing a JavaScript/HTML shell
  around it to actually put it on screen (the browser piece doesn't
  disappear, it just becomes a thinner wrapper), or
- Building a native desktop GUI (Qt, GTK) that is no longer a *web* app
  at all — a fundamentally different kind of program, not a "converted"
  version of this one.

Neither of those is a translation of the existing frontend; both are
different projects. So instead, **`client.cpp` is the honest C++
equivalent**: a console client that calls the exact same HTTP API the
browser frontend calls (`/api/route`, `/api/roads/block`, `/api/journey`,
`/api/route/export`), using the same request/response shapes, just
displayed as text in a terminal instead of pins on a map. It is a real,
working client — tested end-to-end against the server, including
planning a route, running K-shortest, starting and advancing a journey,
and saving a genuine PDF to disk — not a stub.

## File-by-file mapping (JS → C++)

| JS file | C++ file | Notes |
|---|---|---|
| `trafficModel.js` | `trafficModel.hpp` | Direct port, same constants |
| `graph.js` | `graph.hpp` | `EventEmitter` → a `std::vector<std::function<...>>` callback list (same Observer pattern, no framework needed) |
| `algorithms/bfs.js` | `algorithms/bfs.hpp` | Direct port |
| `algorithms/dfs.js` | `algorithms/dfs.hpp` | Direct port |
| `algorithms/dijkstra.js` | `algorithms/dijkstra.hpp` | Hand-rolled `MinHeap` → `std::priority_queue` (STL already has one) |
| `algorithms/kShortestPaths.js` | `algorithms/kShortestPaths.hpp` | Same Yen's-algorithm approach, same temporary-block-then-restore technique |
| `algorithms/RoutingStrategy.js` | `algorithms/RoutingStrategy.hpp` | Real `virtual` functions + abstract base class — genuine polymorphism, not simulated |
| `journey.js` | `journey.hpp` | `#privateFields` → C++ `private:` members (a stronger, compiler-enforced version of the same idea) |
| `persistence.js` | `persistence.hpp` | `fs`/`JSON.stringify` → `nlohmann::json` + `<fstream>` |
| `trafficSimulator.js` | `trafficSimulator.hpp` | `setInterval` (event loop) → a real background `std::thread` |
| `routeExport.js` (pdfkit) | `routeExport.hpp` | No equivalent library installed; **hand-writes valid PDF syntax directly** — verified with `file` and `pdftotext`, not just assumed |
| `osrmClient.js` | `osrmClient.hpp` | `fetch()` → libcurl, same timeout + "return nothing on failure" contract |
| `routeEnrichment.js` | `routeEnrichment.hpp` | Direct port |
| `weatherClient.js` | `weatherClient.hpp` | Direct port |
| `weatherEnrichment.js` | `weatherEnrichment.hpp` | `WeatherCache` Singleton → C++ **private constructor**, so misuse is a *compile error*, not a runtime throw (verified — see below) |
| `middleware/errorHandler.js` | `errorHandler.hpp` | Custom `HttpError` → real C++ exception (`throw`/`catch`), not simulated |
| `routes/route.js` | `routes.hpp` | Express `Router` → `httplib::Server` route registration |
| `server.js` | `server.cpp` | Express app → `cpp-httplib` server; `SIGINT`/`SIGTERM` handled via `signal()` |
| `data/uttarakhandGraph.js` | `data/uttarakhandGraph.hpp` | Same 25 towns, 35 roads, same initial disaster damage |
| `data/historicalEvents.js` | `data/historicalEvents.hpp` | Same two scenarios |
| *(no JS equivalent)* | `client.cpp` | New — the console client explained above |

## Dependencies used (and why)

- **cpp-httplib** (vendored, header-only) — HTTP server; C++ has no
  built-in one the way Node has Express
- **nlohmann/json** — JSON parsing/serialization; C++ has no built-in
  JSON support the way JS does
- **libcurl** — HTTP client for calling OSRM/Open-Meteo; C++ has no
  built-in `fetch()`

## A real bug this conversion caught

While testing, `/api/route` hung indefinitely on every request. Cause:
`for (auto& [k, v] : routeMetrics(...).items())` — the `json` object
returned by `routeMetrics(...)` is a **temporary**, and `.items()`
returns a view that holds a reference into it. In C++, sub-expression
temporaries used to construct a range-for's range-expression are
destroyed at the end of that expression, **before the loop body runs** —
so `v` ended up referencing freed memory. This is a genuine, easy-to-miss
C++ lifetime bug with no JavaScript equivalent (JS has garbage collection
precisely so this class of bug can't happen). Fixed by binding the result
to a named variable first. Documented here rather than silently fixed,
since it's a real example of a bug class C++ introduces that the
original JS code was structurally incapable of having.

## OOP concepts — now including what the earlier C++ console file lacked

The original `cpp-standalone/rescue_path_planner.cpp` (from earlier in
this project) was explicitly missing inheritance and polymorphism. This
conversion adds them for real:

| Concept | Where | Verified how |
|---|---|---|
| Encapsulation | `Journey`'s `private:` fields | Compiles; internal state unreachable except via public methods |
| Abstraction | `RoutingStrategy` pure-virtual interface | `RoutingStrategy s;` is a compile error (abstract class) |
| **Inheritance** | `BfsStrategy`, `DfsStrategy`, `DijkstraStrategy`, `KShortestStrategy` all `: public RoutingStrategy` | Compiles, all four are usable through a base-class pointer |
| **Polymorphism** | `strategy->findRoute(...)` dispatches to the correct override at runtime | Tested: same call, four different behaviors, selected by the object's actual type |
| Singleton | `WeatherCache::getInstance()`, private constructor | **Proven, not claimed** — direct `WeatherCache w;` was compiled and produced `error: 'WeatherCache::WeatherCache()' is private within this context` |
| Observer | `Graph`'s callback list; `Journey` subscribes | Tested: blocking a road the graph knows nothing about "journeys" still correctly triggers a re-route |
| Static factory method | `RoutingStrategy::create(name)` | Tested: correct class returned per name, throws on unknown name |

## Known limitations (same honesty as the rest of this project)

- PDF generation is a minimal hand-written implementation (plain text,
  one page) — real-world use with longer routes or richer formatting
  would benefit from a proper library (e.g. libharu), not attempted here
  to avoid a large new dependency for one feature.
- No automated test suite — every claim in this README was verified with
  a manual compile-and-curl pass during development, documented as such,
  but there's no `make test` target replaying these checks automatically.
- Thread safety: the traffic simulator thread and the HTTP server's
  request-handling threads both touch the same `Graph` object with no
  locking. This mirrors JS's single-threaded safety by luck rather than
  design — Node never has this problem because it's single-threaded;
  C++ does, and this code has not been hardened against it. Under real
  concurrent load this would need a mutex around `Graph` mutations.
