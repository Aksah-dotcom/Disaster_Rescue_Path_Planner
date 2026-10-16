// server.cpp — port of backend/server.js
//
// Wires together everything ported from the JS backend: the graph,
// persistence, traffic simulator, journey manager, and HTTP routes.
// Uses cpp-httplib as the HTTP server (JS used Express).
#include <csignal>
#include <iostream>
#include <atomic>
#include <thread>
#include <curl/curl.h>
#include "vendor/httplib.h"
#include "data/uttarakhandGraph.hpp"
#include "persistence.hpp"
#include "trafficSimulator.hpp"
#include "journey.hpp"
#include "routes.hpp"
#include "routeEnrichment.hpp"
#include "weatherEnrichment.hpp"

std::atomic<bool> g_running{true};
httplib::Server* g_server = nullptr;

void handleSignal(int) {
    std::cout << "\n[shutdown] received signal, closing gracefully...\n";
    g_running = false;
    if (g_server) g_server->stop();
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT); // required once before any libcurl use

    int port = std::getenv("PORT") ? std::atoi(std::getenv("PORT")) : 4000;
    bool enableLiveRouting = !(std::getenv("ENABLE_LIVE_ROUTING") && std::string(std::getenv("ENABLE_LIVE_ROUTING")) == "false");
    bool enableWeather = !(std::getenv("ENABLE_WEATHER") && std::string(std::getenv("ENABLE_WEATHER")) == "false");

    Graph graph = buildUttarakhandGraph();

    Persistence persistence;
    if (auto saved = persistence.load()) {
        persistence.applyTo(graph, *saved);
        std::cout << "[startup] restored persisted road/traffic state\n";
    }
    auto persist = [&]() { persistence.save(graph); };

    TrafficSimulator trafficSim(graph, 8000, 0.3, persist);
    trafficSim.start();

    std::thread weatherThread;
    if (enableWeather) {
        weatherThread = std::thread([&]() {
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::minutes(5));
                if (!g_running) break;
                auto updates = refreshWeatherHazard(graph);
                if (!updates.empty()) {
                    std::cout << "[weather] escalated " << updates.size() << " road(s) due to live rainfall\n";
                    persist();
                }
            }
        });
    }

    JourneyManager journeys;

    httplib::Server svr;
    g_server = &svr;
    registerRoutes(svr, graph, journeys, persist);

    // Phase 2 is API-only — no frontend yet (that's Phase 3), so no
    // static file mount point here.

    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    std::thread enrichmentThread;
    if (enableLiveRouting) {
        enrichmentThread = std::thread([&]() {
            std::cout << "[routing] fetching real road geometry from OSRM in the background...\n";
            enrichGraphWithRealRoutes(graph, [](EnrichmentProgress p) {
                if (p.done % 5 == 0 || p.done == p.total)
                    std::cout << "[routing] " << p.done << "/" << p.total << " (last: " << p.road << " " << (p.ok ? "OK" : "fallback") << ")\n";
            });
            std::cout << "[routing] enrichment pass complete\n";
            persist();
        });
    } else {
        std::cout << "[routing] ENABLE_LIVE_ROUTING=false — using hand-estimated routes only\n";
    }

    std::cout << "Rescue planner (C++) listening on http://localhost:" << port << "\n";
    svr.listen("0.0.0.0", port); // blocks until svr.stop() is called

    std::cout << "[shutdown] stopping traffic simulator...\n";
    trafficSim.stop();
    if (weatherThread.joinable()) weatherThread.join();
    if (enrichmentThread.joinable()) enrichmentThread.join();
    persist();
    curl_global_cleanup();
    std::cout << "[shutdown] server closed\n";
    return 0;
}
