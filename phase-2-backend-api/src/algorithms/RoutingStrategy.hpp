#pragma once
// RoutingStrategy.hpp — port of backend/src/algorithms/RoutingStrategy.js
//
// This is the piece the original C++ console file was explicitly missing:
// real inheritance and polymorphism. RoutingStrategy is an ABSTRACT base
// class (pure virtual findRoute()) — every concrete strategy overrides
// it, and calling code invokes findRoute() through a base-class pointer
// without knowing or caring which concrete algorithm is behind it. That
// dynamic dispatch (decided at RUNTIME based on the object's actual
// type, not at compile time) is what makes this genuine polymorphism,
// not just four unrelated functions that happen to share a name.
#include <memory>
#include <stdexcept>
#include "bfs.hpp"
#include "dfs.hpp"
#include "dijkstra.hpp"
#include "kShortestPaths.hpp"

// [OOP: Abstract base class] Pure virtual findRoute() means RoutingStrategy
// can never be instantiated directly (`RoutingStrategy s;` is a compile
// error) — it only exists to define the interface every subclass must
// implement. This is C++'s equivalent of the JS base class throwing
// "must be implemented by a subclass".
class RoutingStrategy {
public:
    virtual ~RoutingStrategy() = default;
    virtual RouteResult findRoute(Graph& g, const std::string& start, const std::string& end) = 0;
    virtual std::string name() const = 0;

    // [OOP: Static factory method] The one place that maps a string name
    // to a concrete class — mirrors RoutingStrategy.create(name) in JS.
    static std::unique_ptr<RoutingStrategy> create(const std::string& name);
    static std::vector<std::string> availableNames();
};

// [OOP: Inheritance + Polymorphism] Each of these four classes `: public
// RoutingStrategy` and OVERRIDES findRoute() with genuinely different
// logic. A `RoutingStrategy*` or `unique_ptr<RoutingStrategy>` pointing
// at any of these four will call the CORRECT version of findRoute()
// automatically at runtime — that's virtual dispatch, the mechanism
// polymorphism relies on.
class BfsStrategy : public RoutingStrategy {
public:
    RouteResult findRoute(Graph& g, const std::string& s, const std::string& e) override { return bfs(g, s, e); }
    std::string name() const override { return "bfs"; }
};

class DfsStrategy : public RoutingStrategy {
public:
    RouteResult findRoute(Graph& g, const std::string& s, const std::string& e) override { return dfs(g, s, e); }
    std::string name() const override { return "dfs"; }
};

class DijkstraStrategy : public RoutingStrategy {
public:
    RouteResult findRoute(Graph& g, const std::string& s, const std::string& e) override { return dijkstra(g, s, e); }
    std::string name() const override { return "dijkstra"; }
};

class KShortestStrategy : public RoutingStrategy {
public:
    std::vector<KRoute> findRoutes(Graph& g, const std::string& s, const std::string& e, int k = 3) {
        return kShortestPaths(g, s, e, k);
    }
    RouteResult findRoute(Graph& g, const std::string& s, const std::string& e) override {
        auto routes = findRoutes(g, s, e, 1);
        RouteResult r;
        if (!routes.empty()) { r.path = routes[0].path; r.cost = routes[0].cost; r.hasCost = true; }
        return r;
    }
    std::string name() const override { return "kshortest"; }
};

inline std::unique_ptr<RoutingStrategy> RoutingStrategy::create(const std::string& n) {
    if (n == "bfs") return std::make_unique<BfsStrategy>();
    if (n == "dfs") return std::make_unique<DfsStrategy>();
    if (n == "dijkstra") return std::make_unique<DijkstraStrategy>();
    if (n == "kshortest") return std::make_unique<KShortestStrategy>();
    throw std::invalid_argument("Unknown routing algorithm: " + n);
}

inline std::vector<std::string> RoutingStrategy::availableNames() {
    return {"bfs", "dfs", "dijkstra", "kshortest"};
}
