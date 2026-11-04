#pragma once

#include <queue>
#include <limits>
#include "bfs.hpp"

inline RouteResult dijkstra(const Graph& g, const std::string& start, const std::string& end) {

    std::unordered_map<std::string, double> dist;
    std::unordered_map<std::string, std::string> parent;
    std::unordered_set<std::string> visited;
    std::vector<std::string> order;

    for (const auto& id : g.nodeOrder) {
        dist[id] = std::numeric_limits<double>::infinity();
    }

    dist[start] = 0.0;

    using PQItem = std::pair<double, std::string>;

    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<>> pq;
    pq.emplace(0.0, start);

    while (!pq.empty()) {

        auto [d, u] = pq.top();
        pq.pop();

        if (visited.find(u) != visited.end()) {
            continue;
        }

        visited.insert(u);
        order.push_back(u);

        if (u == end) {
            break;
        }

        for (const auto& e : g.neighbors(u)) {

            double nd = d + e.weight;

            if (nd < dist[e.to]) {
                parent[e.to] = u;
                dist[e.to] = nd;
                pq.emplace(nd, e.to);
            }
        }
    }

    RouteResult r;

    double finalDist = std::numeric_limits<double>::infinity();

    if (dist.find(end) != dist.end()) {
        finalDist = dist[end];
    }

    if (finalDist < std::numeric_limits<double>::infinity()) {

        auto visitedPlusEnd = visited;
        visitedPlusEnd.insert(end);

        r.path = buildPath(parent, start, end, visitedPlusEnd);
        r.cost = finalDist;
        r.hasCost = true;

    } else {
        r.hasCost = false;
    }

    r.order = order;

    return r;
}