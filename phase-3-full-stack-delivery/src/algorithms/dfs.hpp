
#pragma once
// dfs.hpp — DFS implementation
#include "bfs.hpp" // reuse RouteResult + buildPath

inline bool dfsWalk(const Graph& g, const std::string& current, const std::string& end,
                    std::unordered_set<std::string>& visited, std::vector<std::string>& order,
                    std::unordered_map<std::string,std::string>& parent) {

    visited.insert(current);
    order.push_back(current);

    if (current == end) return true;

    for (auto& e : g.neighbors(current)) {
        if (visited.count(e.to)) continue;

        parent[e.to] = current;

        if (dfsWalk(g, e.to, end, visited, order, parent))
            return true;
    }

    return false;
}

inline RouteResult dfs(const Graph& g, const std::string& start, const std::string& end) {
    std::unordered_set<std::string> visited;
    std::vector<std::string> order;
    std::unordered_map<std::string,std::string> parent;

    dfsWalk(g, start, end, visited, order, parent);

    RouteResult result;
    result.path = buildPath(parent, start, end, visited);
    result.order = order;
    result.hasCost = false;

    return result;
}
