
#pragma once
// bfs.hpp — BFS implementation
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <algorithm>
#include "../graph.hpp"

struct RouteResult {
    std::vector<std::string> path;
    std::vector<std::string> order;
    double cost = -1;
    bool hasCost = false;
};

inline std::vector<std::string> buildPath(
    const std::unordered_map<std::string,std::string>& parent,
    const std::string& start, const std::string& end,
    const std::unordered_set<std::string>& visited)
{
    if (!visited.count(end) && end != start) return {};

    std::vector<std::string> path{end};
    std::string current = end;

    while (current != start) {
        auto it = parent.find(current);
        if (it == parent.end()) return {};

        current = it->second;
        path.push_back(current);
    }

    std::reverse(path.begin(), path.end());
    return path;
}

inline RouteResult bfs(const Graph& g, const std::string& start, const std::string& end) {
    std::unordered_map<std::string,std::string> parent;
    std::unordered_set<std::string> visited{start};
    std::vector<std::string> order;
    std::deque<std::string> queue{start};

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop_front();

        order.push_back(current);

        if (current == end) break;

        for (auto& e : g.neighbors(current)) {
            if (visited.count(e.to)) continue;

            visited.insert(e.to);
            parent[e.to] = current;
            queue.push_back(e.to);
        }
    }

    RouteResult result;
    result.path = buildPath(parent, start, end, visited);
    result.order = order;
    result.hasCost = false;

    return result;
}
