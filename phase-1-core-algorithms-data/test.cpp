/**
 * Proves Phase 1 works with NO server, NO external services, NO
 * dependencies beyond a C++17 compiler and nlohmann/json (already
 * required transitively by graph.hpp's toJSON, but nothing here makes
 * a network call or starts a server).
 *
 * Build: g++ -std=c++17 -I src test.cpp -o test
 * Run:   ./test
 */
#include <iostream>
#include <set>
#include <deque>
#include "src/data/uttarakhandGraph.hpp"
#include "src/algorithms/RoutingStrategy.hpp"
#include "src/journey.hpp"

int main() {
    Graph g = buildUttarakhandGraph();
    std::cout << "Graph loaded: " << g.nodes.size() << " towns\n\n";

    std::cout << "--- All four routing strategies (polymorphism) ---\n";
    for (auto& n : RoutingStrategy::availableNames()) {
        auto strategy = RoutingStrategy::create(n);
        auto r = strategy->findRoute(g, "DEH", "BAD");
        std::cout << strategy->name() << ": ";
        for (auto& x : r.path) std::cout << x << " ";
        std::cout << "\n";
    }

    std::cout << "\n--- K-shortest paths (top 3) ---\n";
    KShortestStrategy kss;
    for (auto& r : kss.findRoutes(g, "DEH", "BAD", 3))
        std::cout << "  #" << r.rank << " cost=" << r.cost << "\n";

    std::cout << "\n--- Dynamic re-routing (Observer pattern) ---\n";
    JourneyManager mgr;
    auto j = mgr.start(g, "DEH", "BAD", "dijkstra");
    j->advance(); j->advance();
    std::cout << "Team at: " << j->currentNode() << "\n";
    auto next = j->remainingPath()[1];
    std::cout << "Blocking road ahead: " << j->currentNode() << " -> " << next << "\n";
    g.setBlocked(j->currentNode(), next, true, "test");
    std::cout << "Team position after reroute: " << j->currentNode() << " (should be unchanged)\n";

    std::cout << "\n--- Connectivity check ---\n";
    std::set<std::string> seen{"DEH"};
    std::deque<std::string> q{"DEH"};
    while (!q.empty()) {
        auto u = q.front(); q.pop_front();
        for (auto& e : g.neighbors(u)) if (!seen.count(e.to)) { seen.insert(e.to); q.push_back(e.to); }
    }
    std::cout << seen.size() << "/" << g.nodes.size() << " towns reachable from DEH\n";
    return 0;
}
