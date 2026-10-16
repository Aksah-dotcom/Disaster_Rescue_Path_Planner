#pragma once
// bfs.hpp — port of backend/src/algorithms/bfs.js
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <algorithm>
#include "../graph.hpp"

struct RouteResult {
    std::vector<std::string> path;
    std::vector<std::string> order;   // visitation order, for animating the search
    double cost = -1;                 // -1 means "no cost" (null), used by BFS/DFS
    bool hasCost = false;
};

inline std::vector<std::string> buildPath(
    const std::unordered_map<std::string,std::string>& parent,
    const std::string& start, const std::string& end,
    const std::unordered_set<std::string>& visited)
{
    if (!visited.count(end) && end != start) return {};
    std::vector<std::string> path{end};
    std::string cur = end;
    while (cur != start) {
        auto it = parent.find(cur);
        if (it == parent.end()) return {};
        cur = it->second;
        path.push_back(cur);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// [Not a class method — a free function, matching the JS module's shape,
//  which itself exports a plain function rather than a class.]
inline RouteResult bfs(const Graph& g, const std::string& start, const std::string& end) {
    std::unordered_map<std::string,std::string> parent;
    std::unordered_set<std::string> visited{start};
    std::vector<std::string> order;
    std::deque<std::string> queue{start};

    while (!queue.empty()) {
        std::string u = queue.front(); queue.pop_front();
        order.push_back(u);
        if (u == end) break;
        for (auto& e : g.neighbors(u)) {
            if (visited.count(e.to)) continue;
            visited.insert(e.to);
            parent[e.to] = u;
            queue.push_back(e.to);
        }
    }
    RouteResult r;
    r.path = buildPath(parent, start, end, visited);
    r.order = order;
    r.hasCost = false;
    return r;
}
