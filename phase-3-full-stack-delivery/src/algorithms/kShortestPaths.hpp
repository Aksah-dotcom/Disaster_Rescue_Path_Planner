#pragma once
// kShortestPaths.hpp — port of backend/src/algorithms/kShortestPaths.js
// Same approach as the JS version: temporarily block edges/nodes using
// the graph's own setBlocked(), run Dijkstra from the spur node, then
// restore everything — never a separate masking mechanism.
#include <set>
#include "dijkstra.hpp"

struct KRoute { std::vector<std::string> path; double cost; int rank; };

inline double pathCost(const Graph& g, const std::vector<std::string>& path) {
    double cost = 0;
    for (size_t i = 0; i + 1 < path.size(); i++) {
        auto e = g.getEdgeInfo(path[i], path[i+1]);
        if (e) cost += e->weight;
    }
    return cost;
}

inline std::vector<KRoute> kShortestPaths(Graph& g, const std::string& start, const std::string& end, int k = 3) {
    auto first = dijkstra(g, start, end);
    if (first.path.empty()) return {};

    std::vector<RouteResult> A{first};
    std::vector<std::pair<std::vector<std::string>, double>> B;

    for (int ki = 1; ki < k; ki++) {
        auto& prevPath = A[ki - 1].path;
        for (size_t i = 0; i + 1 < prevPath.size(); i++) {
            std::string spurNode = prevPath[i];
            std::vector<std::string> rootPath(prevPath.begin(), prevPath.begin() + i + 1);

            std::vector<std::pair<std::string,std::string>> removed;

            // remove edges that would just recreate an already-found path sharing this root
            for (auto& r : A) {
                if (r.path.size() > i && std::equal(rootPath.begin(), rootPath.end(), r.path.begin())) {
                    if (i + 1 < r.path.size()) {
                        std::string a = r.path[i], b = r.path[i+1];
                        auto e = g.getEdgeInfo(a, b);
                        if (e && !e->blocked) { g.setBlocked(a, b, true, "__yen_temp__"); removed.push_back({a,b}); }
                    }
                }
            }
            // remove root-path nodes (except spur) by blocking their incident edges
            for (size_t j = 0; j + 1 < rootPath.size(); j++) {
                std::string nodeId = rootPath[j];
                for (auto& e : g.adj[nodeId]) {
                    if (!e.blocked) { g.setBlocked(nodeId, e.to, true, "__yen_temp__"); removed.push_back({nodeId, e.to}); }
                }
            }

            auto spurResult = dijkstra(g, spurNode, end);

            for (auto& [a, b] : removed) g.setBlocked(a, b, false);

            if (!spurResult.path.empty()) {
                std::vector<std::string> totalPath(rootPath.begin(), rootPath.end() - 1);
                totalPath.insert(totalPath.end(), spurResult.path.begin(), spurResult.path.end());
                double totalCost = pathCost(g, rootPath) + spurResult.cost;

                bool dup = false;
                for (auto& r : A) if (r.path == totalPath) dup = true;
                for (auto& b2 : B) if (b2.first == totalPath) dup = true;
                if (!dup) B.push_back({totalPath, totalCost});
            }
        }
        if (B.empty()) break;
        std::sort(B.begin(), B.end(), [](auto& x, auto& y){ return x.second < y.second; });
        RouteResult next; next.path = B.front().first; next.cost = B.front().second; next.hasCost = true;
        A.push_back(next);
        B.erase(B.begin());
    }

    std::vector<KRoute> out;
    for (size_t i = 0; i < A.size(); i++) out.push_back({A[i].path, A[i].cost, (int)i + 1});
    return out;
}
