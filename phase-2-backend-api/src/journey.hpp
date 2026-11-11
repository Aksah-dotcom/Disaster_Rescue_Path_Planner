#pragma once
// journey.hpp — port of backend/src/journey.js
//
// Journey subscribes to the Graph's block-change notifications (Observer
// pattern) — if a road still ahead of the team gets blocked, it
// recomputes the remaining route from the team's ACTUAL current
// position, not the original start.
#include <random>
#include <sstream>
#include "algorithms/RoutingStrategy.hpp"

inline std::string makeSimpleId() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 15);
    const char* hex = "0123456789abcdef";
    std::ostringstream oss;
    for (int i = 0; i < 16; i++) oss << hex[dist(rng)];
    return oss.str();
}

struct RerouteEvent { std::string reason, from; bool found; };

// [OOP: Encapsulation via private members] Journey's internal state
// (path, position, algorithm) is private and only reachable through the
// public methods/getters below — the C++ equivalent of the JS version's
// #privateFields.
class Journey {
public:
    std::string id;

    Journey(Graph& graph, std::vector<std::string> path, std::string algorithm, std::string destination)
        : id(makeSimpleId()), _graph(graph), _path(std::move(path)),
          _algorithm(std::move(algorithm)), _destination(std::move(destination)) {}

    //std::string currentNode() const { return _path[_pos]; }
    std::string currentNode() const {
    if (_path.empty() || _pos >= _path.size()) return "";
    return _path[_pos];
}
    std::vector<std::string> remainingPath() const { return {_path.begin() + _pos, _path.end()}; }
    std::vector<std::string> fullPath() const { return _path; }
    bool isComplete() const { return currentNode() == _destination; }
    const std::vector<RerouteEvent>& rerouteHistory() const { return _rerouteLog; }

    std::string advance() {
        if (_pos + 1 < (int)_path.size()) _pos++;
        return currentNode();
    }

    // Called by the graph's observer notification. Returns true if a
    // reroute actually happened.
    bool handleBlock(const std::string& a, const std::string& b) {
        if (isComplete()) return false;
        auto remaining = remainingPath();
        bool stillNeeded = false;
        for (size_t i = 0; i + 1 < remaining.size(); i++) {
            if ((remaining[i] == a && remaining[i+1] == b) || (remaining[i] == b && remaining[i+1] == a)) {
                stillNeeded = true; break;
            }
        }
        if (!stillNeeded) return false;

        auto strategy = RoutingStrategy::create(_algorithm);
        auto result = strategy->findRoute(_graph, currentNode(), _destination);
        _rerouteLog.push_back({"road " + a + "-" + b + " blocked", currentNode(), !result.path.empty()});

        if (!result.path.empty()) {
            std::vector<std::string> newPath(_path.begin(), _path.begin() + _pos);
            newPath.insert(newPath.end(), result.path.begin(), result.path.end());
            _path = newPath;
        }
        return true;
    }

private:
    Graph& _graph;
    std::vector<std::string> _path;
    int _pos = 0;
    std::string _algorithm, _destination;
    std::vector<RerouteEvent> _rerouteLog;
};

// JourneyManager — owns the collection, keeps Journey unaware that
// multiple journeys can exist (single responsibility split).
class JourneyManager {
public:
    std::shared_ptr<Journey> start(Graph& g, const std::string& startId, const std::string& destination, const std::string& algorithm) {
        auto strategy = RoutingStrategy::create(algorithm);
        auto result = strategy->findRoute(g, startId, destination);
        if (result.path.empty()) return nullptr;

        auto journey = std::make_shared<Journey>(g, result.path, algorithm, destination);
        g.onRoadBlockChange([journey](const BlockEvent& ev) {
            if (ev.blocked) journey->handleBlock(ev.a, ev.b);
        });
        _journeys[journey->id] = journey;
        return journey;
    }

    std::shared_ptr<Journey> get(const std::string& id) {
        auto it = _journeys.find(id);
        return it != _journeys.end() ? it->second : nullptr;
    }

    bool end(const std::string& id) { return _journeys.erase(id) > 0; }

private:
    std::unordered_map<std::string, std::shared_ptr<Journey>> _journeys;
};
