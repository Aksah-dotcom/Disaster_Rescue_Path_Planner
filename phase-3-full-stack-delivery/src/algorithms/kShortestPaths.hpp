#pragma once

// kShortestPaths.hpp
// Deeksha's contribution: Improved K-Shortest Paths Algorithm
// Algorithm: Yen's Algorithm + Dijkstra

#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <utility>

#include "dijkstra.hpp"

struct KRoute {
    std::vector<std::string> path;
    double cost;
    int rank;
};


// Calculate total cost of a path
inline double pathCost(
    const Graph& g,
    const std::vector<std::string>& path
) {

    double cost = 0;

    for (size_t i = 0; i + 1 < path.size(); i++) {

        auto e = g.getEdgeInfo(path[i], path[i + 1]);

        if (!e)
            return std::numeric_limits<double>::infinity();

        cost += e->weight;
    }

    return cost;
}


// Check whether a path already exists
inline bool isDuplicatePath(
    const std::vector<std::string>& path,
    const std::vector<RouteResult>& routes
) {

    for (const auto& r : routes) {

        if (r.path == path)
            return true;
    }

    return false;
}


// Check duplicate candidate paths
inline bool isDuplicateCandidate(
    const std::vector<std::string>& path,
    const std::vector<std::pair<std::vector<std::string>, double>>& candidates
) {

    for (const auto& c : candidates) {

        if (c.first == path)
            return true;
    }

    return false;
}


// K Shortest Paths using Yen's Algorithm
inline std::vector<KRoute> kShortestPaths(
    Graph& g,
    const std::string& start,
    const std::string& end,
    int k = 3
) {

    if (k <= 0)
        return {};

    auto first = dijkstra(g, start, end);

    if (first.path.empty())
        return {};

    std::vector<RouteResult> A{first};

    std::vector<
        std::pair<std::vector<std::string>, double>
    > B;

    for (int ki = 1; ki < k; ki++) {

        auto& prevPath = A[ki - 1].path;

        for (size_t i = 0; i + 1 < prevPath.size(); i++) {

            std::string spurNode = prevPath[i];

            std::vector<std::string> rootPath(
                prevPath.begin(),
                prevPath.begin() + i + 1
            );

            std::vector<
                std::pair<std::string, std::string>
            > removed;

            // Remove edges used by previously accepted routes
            for (auto& r : A) {

                if (
                    r.path.size() > i + 1 &&
                    std::equal(
                        rootPath.begin(),
                        rootPath.end(),
                        r.path.begin()
                    )
                ) {

                    std::string a = r.path[i];
                    std::string b = r.path[i + 1];

                    auto e = g.getEdgeInfo(a, b);

                    if (e && !e->blocked) {

                        g.setBlocked(
                            a, b, true, "__yen_temp__"
                        );

                        removed.push_back({a, b});
                    }
                }
            }

            // Temporarily block root-path nodes
            for (size_t j = 0; j + 1 < rootPath.size(); j++) {

                std::string nodeId = rootPath[j];

                for (auto& e : g.adj[nodeId]) {

                    if (!e.blocked) {

                        g.setBlocked(
                            nodeId,
                            e.to,
                            true,
                            "__yen_temp__"
                        );

                        removed.push_back({
                            nodeId,
                            e.to
                        });
                    }
                }
            }

            // Find shortest spur path
            auto spurResult = dijkstra(
                g,
                spurNode,
                end
            );

            // Restore temporarily blocked edges
            for (auto& edge : removed) {

                g.setBlocked(
                    edge.first,
                    edge.second,
                    false
                );
            }

            if (!spurResult.path.empty()) {

                std::vector<std::string> totalPath(
                    rootPath.begin(),
                    rootPath.end() - 1
                );

                totalPath.insert(
                    totalPath.end(),
                    spurResult.path.begin(),
                    spurResult.path.end()
                );

                double totalCost =
                    pathCost(g, rootPath) + spurResult.cost;

                // Avoid duplicate routes
                if (
                    !isDuplicatePath(totalPath, A) &&
                    !isDuplicateCandidate(totalPath, B)
                ) {

                    B.push_back({
                        totalPath,
                        totalCost
                    });
                }
            }
        }

        if (B.empty())
            break;

        // Sort candidates by cost
        std::sort(
            B.begin(),
            B.end(),
            [](const auto& x, const auto& y) {

                if (x.second != y.second)
                    return x.second < y.second;

                return x.first < y.first;
            }
        );

        // Select the next shortest route
        RouteResult next;

        next.path = B.front().first;
        next.cost = B.front().second;
        next.hasCost = true;

        A.push_back(next);

        B.erase(B.begin());
    }

    // Prepare final ranked routes
    std::vector<KRoute> result;

    for (size_t i = 0; i < A.size(); i++) {

        result.push_back({
            A[i].path,
            A[i].cost,
            static_cast<int>(i) + 1
        });
    }

    return result;
}