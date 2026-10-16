#pragma once
// dfs.hpp — port of backend/src/algorithms/dfs.js
#include "bfs.hpp" // reuse RouteResult + buildPath

inline bool dfsWalk(const Graph& g, const std::string& u, const std::string& end,
                     std::unordered_set<std::string>& visited, std::vector<std::string>& order,
                     std::unordered_map<std::string,std::string>& parent) {
    visited.insert(u);
    order.push_back(u);
    if (u == end) return true;
    for (auto& e : g.neighbors(u)) {
        if (visited.count(e.to)) continue;
        parent[e.to] = u;
        if (dfsWalk(g, e.to, end, visited, order, parent)) return true;
    }
    return false;
}

inline RouteResult dfs(const Graph& g, const std::string& start, const std::string& end) {
    std::unordered_set<std::string> visited;
    std::vector<std::string> order;
    std::unordered_map<std::string,std::string> parent;
    dfsWalk(g, start, end, visited, order, parent);

    RouteResult r;
    r.path = buildPath(parent, start, end, visited);
    r.order = order;
    r.hasCost = false;
    return r;
}
