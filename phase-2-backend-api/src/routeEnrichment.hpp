#pragma once
// routeEnrichment.hpp — port of backend/src/routeEnrichment.js
#include <thread>
#include <chrono>
#include <functional>
#include "osrmClient.hpp"

struct EnrichmentProgress { int done, total, succeeded; std::string road; bool ok; };

inline void enrichGraphWithRealRoutes(Graph& g, std::function<void(EnrichmentProgress)> onProgress = nullptr, int delayMs = 300) {
    std::vector<std::tuple<std::string,std::string,std::string>> targets; // edgeId, fromId, toId
    std::set<std::string> seen;
    for (auto& [u, list] : g.adj) {
        for (auto& e : list) {
            if (seen.count(e.edgeId) || e.real) continue;
            seen.insert(e.edgeId);
            targets.push_back({e.edgeId, u, e.to});
        }
    }

    int done = 0, succeeded = 0;
    for (auto& [edgeId, fromId, toId] : targets) {
        auto result = fetchRealRoute(g.nodes[fromId], g.nodes[toId]);
        if (result) {
            g.setRealRoute(edgeId, fromId, result->geometry, result->distanceKm);
            succeeded++;
        }
        done++;
        if (onProgress) onProgress({done, (int)targets.size(), succeeded, fromId + "-" + toId, result.has_value()});
        if (delayMs) std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}
