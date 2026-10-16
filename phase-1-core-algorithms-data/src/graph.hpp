#pragma once
// graph.hpp — C++ port of backend/src/graph.js
//
// Weighted, undirected graph with dynamic road blocking and live traffic.
// The JS version extends Node's EventEmitter so it can emit "road-blocked"
// events; C++ has no built-in EventEmitter, so this uses the same idea
// implemented directly: a list of std::function callbacks that get
// invoked from setBlocked(). This is still the Observer pattern — same
// concept, no framework needed.

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <algorithm>
#include <set>
#include <tuple>
#include <nlohmann/json.hpp>
#include "trafficModel.hpp"

struct Node {
    std::string id, label, sub;
    double lat, lng;
};

struct Edge {
    std::string to;       // destination node id
    std::string road;
    double distanceKm;
    double riskFactor;
    double weight;         // distanceKm * riskFactor
    std::string trafficLevel; // "low" | "medium" | "high"
    bool blocked = false;
    std::string blockReason;
    bool real = false;      // true once real OSRM data replaces the estimate
    std::vector<std::pair<double,double>> waypoints;
    std::string edgeId;
};

// Observer pattern: payload passed to subscribers on block/unblock
struct BlockEvent {
    std::string a, b, reason;
    bool blocked;
};

class Graph {
public:
    std::vector<std::string> nodeOrder;                  // preserves insertion order
    std::unordered_map<std::string, Node> nodes;
    std::unordered_map<std::string, std::vector<Edge>> adj;

    void addNode(const std::string& id, const std::string& label, const std::string& sub, double lat, double lng) {
        nodes[id] = {id, label, sub, lat, lng};
        if (adj.find(id) == adj.end()) { adj[id] = {}; nodeOrder.push_back(id); }
    }

    std::string addRoad(const std::string& a, const std::string& b, const std::string& road,
                         double distanceKm, double riskFactor, const std::string& trafficLevel,
                         bool blocked = false, const std::string& blockReason = "",
                         std::vector<std::pair<double,double>> waypoints = {}) {
        std::string edgeId = a + "__" + b + "__" + std::to_string(_edgeSeq++);
        double weight = std::round(distanceKm * riskFactor * 10.0) / 10.0;
        Edge eAB{b, road, distanceKm, riskFactor, weight, trafficLevel, blocked, blockReason, false, waypoints, edgeId};
        Edge eBA{a, road, distanceKm, riskFactor, weight, trafficLevel, blocked, blockReason, false, waypoints, edgeId};
        adj[a].push_back(eAB);
        adj[b].push_back(eBA);
        return edgeId;
    }

    // [OOP: Observer pattern] Subscribe to block/unblock events. Graph
    // has zero awareness of who's listening (e.g. Journey) — this is the
    // same decoupling the JS version gets from EventEmitter.
    void onRoadBlockChange(std::function<void(const BlockEvent&)> cb) {
        _listeners.push_back(cb);
    }

    bool setBlocked(const std::string& a, const std::string& b, bool blocked, const std::string& reason = "") {
        bool found = false;
        for (auto& e : adj[a]) if (e.to == b) { e.blocked = blocked; e.blockReason = blocked ? reason : ""; found = true; }
        for (auto& e : adj[b]) if (e.to == a) { e.blocked = blocked; e.blockReason = blocked ? reason : ""; found = true; }
        if (found) {
            BlockEvent ev{a, b, reason, blocked};
            for (auto& cb : _listeners) cb(ev); // notify every subscriber
        }
        return found;
    }

    bool setTraffic(const std::string& a, const std::string& b, const std::string& level) {
        bool found = false;
        for (auto& e : adj[a]) if (e.to == b) { e.trafficLevel = level; found = true; }
        for (auto& e : adj[b]) if (e.to == a) { e.trafficLevel = level; found = true; }
        return found;
    }

    // Replaces an edge's estimate with real routing-engine data (see osrmClient).
    void setRealRoute(const std::string& edgeId, const std::string& fromId,
                       std::vector<std::pair<double,double>> geometry, double distanceKm) {
        for (auto& [nodeId, list] : adj) {
            for (auto& e : list) {
                if (e.edgeId != edgeId) continue;
                e.distanceKm = distanceKm;
                e.weight = std::round(distanceKm * e.riskFactor * 10.0) / 10.0;
                e.real = true;
                if (nodeId == fromId) e.waypoints = geometry;
                else { auto rev = geometry; std::reverse(rev.begin(), rev.end()); e.waypoints = rev; }
            }
        }
    }

    std::optional<Edge> getEdgeInfo(const std::string& a, const std::string& b) const {
        auto it = adj.find(a);
        if (it != adj.end())
            for (auto& e : it->second) if (e.to == b) return e;
        return std::nullopt;
    }

    // returns only unblocked neighbors — exactly like graph.js's neighbors()
    std::vector<Edge> neighbors(const std::string& id) const {
        std::vector<Edge> out;
        auto it = adj.find(id);
        if (it == adj.end()) return out;
        for (auto& e : it->second) if (!e.blocked) out.push_back(e);
        return out;
    }

    bool hasNode(const std::string& id) const { return nodes.count(id) > 0; }

    std::vector<std::tuple<std::string,std::string,std::string>> allUnblockedEdgeEndpoints() const {
        std::vector<std::tuple<std::string,std::string,std::string>> out;
        for (auto& [u, list] : adj)
            for (auto& e : list)
                if (!e.blocked) out.push_back({u, e.to, e.edgeId});
        return out;
    }

    // Serializes the graph to the same JSON shape the JS backend's
    // toJSON() produces, so a frontend written against that shape needs
    // zero changes to talk to this server instead.
    nlohmann::json toJSON() const {
        nlohmann::json nodesArr = nlohmann::json::array();
        for (auto& id : nodeOrder) {
            auto& n = nodes.at(id);
            nodesArr.push_back({{"id", n.id}, {"label", n.label}, {"sub", n.sub}, {"lat", n.lat}, {"lng", n.lng}});
        }

        nlohmann::json edgesArr = nlohmann::json::array();
        std::set<std::string> seen;
        for (auto& u : nodeOrder) {
            for (auto& e : adj.at(u)) {
                if (seen.count(e.edgeId)) continue;
                seen.insert(e.edgeId);
                nlohmann::json waypoints = nlohmann::json::array();
                for (auto& [lat, lng] : e.waypoints) waypoints.push_back({lat, lng});
                double travelTime = std::round(trafficModel::travelTimeMinutes(e.distanceKm, e.trafficLevel) * 10.0) / 10.0;
                edgesArr.push_back({
                    {"a", u}, {"b", e.to}, {"road", e.road}, {"weight", e.weight},
                    {"blocked", e.blocked}, {"blockReason", e.blockReason},
                    {"distanceKm", e.distanceKm}, {"trafficLevel", e.trafficLevel},
                    {"waypoints", waypoints}, {"real", e.real}, {"travelTimeMin", travelTime}
                });
            }
        }
        return {{"nodes", nodesArr}, {"edges", edgesArr}};
    }

private:
    int _edgeSeq = 0;
    std::vector<std::function<void(const BlockEvent&)>> _listeners;
};
