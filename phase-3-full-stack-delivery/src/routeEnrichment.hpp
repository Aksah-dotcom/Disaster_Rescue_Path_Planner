#pragma once
// routeEnrichment.hpp — port of backend/src/routeEnrichment.js
//fetches real road routes using OSRM and updates the graph
#include <thread>
#include <chrono>
#include <vector>
#include <tuple>
#include <set>
#include <string>
#include <functional>

#include "osrmClient.hpp"

struct EnrichmentProgress { 
    int done;
    int total;
    int succeeded; 
    std::string road; 
    bool ok; 
};

inline void enrichGraphWithRealRoutes(
    Graph& g, 
    std::function<void(EnrichmentProgress)> onProgress = nullptr, 
    int delayMs = 300
) {
    std::vector<std::tuple<std::string,std::string,std::string>> targets; 
    // edgeId, fromId, toId
    std::set<std::string> seen;

    //Collect edges that need real route geometry
    for (auto& [fromId, edges] : g.adj) {
        for (auto& edge : edges) {
            //skips duplicate and already enriched edges
            if (seen.count(edge.edgeId) || 
            (edge.real && !edge.waypoints.empty())) { 
                continue;
            }

            seen.insert(edge.edgeId);
            targets.push_back({edge.edgeId, fromId, edge.to});
        }
    }

    int done = 0, succeeded = 0;
    int total = static_cast<int>(targets.size());

    for (auto& [edgeId, fromId, toId] : targets) {
        auto result = fetchRealRoute(g.nodes[fromId], g.nodes[toId]);

        if (result) {
            g.setRealRoute(
                edgeId, 
                fromId, 
                result->geometry, 
                result->distanceKm
            );
            succeeded++;
        }
        done++;

        if (onProgress) {
            onProgress({
                done,
                total,
                succeeded,
                fromId + "-" + toId,
                result.has_value()
            });
        }
        if (delayMs > 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(delayMs)
            );
        }
    }
}