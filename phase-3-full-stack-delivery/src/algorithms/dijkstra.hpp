#pragma once
// dijkstra.hpp — port of backend/src/algorithms/dijkstra.js
//
// The JS version hand-rolls a binary min-heap (MinHeap class). C++'s STL
// already provides one: std::priority_queue. Using it here is the more
// idiomatic C++ choice — same O((V+E) log V) complexity, no need to
// reimplement heap mechanics that the standard library already gives you.
#include <queue>
#include <limits>
#include "bfs.hpp"

inline RouteResult dijkstra(const Graph& g, const std::string& start, const std::string& end) {
    std::unordered_map<std::string,double> dist;
    std::unordered_map<std::string,std::string> parent;
    std::unordered_set<std::string> visited;
    std::vector<std::string> order;

    for (auto& id : g.nodeOrder) dist[id] = std::numeric_limits<double>::infinity();
    dist[start] = 0;

    using PQItem = std::pair<double,std::string>; // (distance, nodeId)
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<>> pq;
    pq.push({0.0, start});

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (visited.count(u)) continue;
        visited.insert(u);
        order.push_back(u);
        if (u == end) break;

        for (auto& e : g.neighbors(u)) {
            double nd = d + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }

    RouteResult r;
    double finalDist = dist.count(end) ? dist[end] : std::numeric_limits<double>::infinity();
    if (finalDist != std::numeric_limits<double>::infinity()) {
        auto visitedPlusEnd = visited; visitedPlusEnd.insert(end);
        r.path = buildPath(parent, start, end, visitedPlusEnd);
        r.cost = finalDist;
        r.hasCost = true;
    } else {
        r.hasCost = false;
    }
    r.order = order;
    return r;
}
