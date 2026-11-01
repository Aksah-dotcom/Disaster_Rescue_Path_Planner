#pragma once
// persistence.hpp — port of backend/src/persistence.js
// JS used raw fs.readFile/writeFile with JSON.stringify/parse; here we
// use nlohmann::json, the de-facto standard JSON library for C++.
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "graph.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

class Persistence {
public:
    explicit Persistence(std::string dataDir = "data") : _dataDir(std::move(dataDir)) {
        _stateFile = _dataDir + "/state.json";
    }

    // Returns std::nullopt if no saved state exists yet (fresh start).
    std::optional<json> load() {
        if (!fs::exists(_stateFile)) return std::nullopt;
        try {
            std::ifstream in(_stateFile);
            json j; in >> j;
            return j;
        } catch (...) {
            return std::nullopt; // corrupt file — start fresh rather than crash
        }
    }

    void save(const Graph& g) {
        if (!fs::exists(_dataDir)) fs::create_directories(_dataDir);
        json edges = json::object();
        std::set<std::string> seen;
        for (auto& [u, list] : g.adj) {
            for (auto& e : list) {
                if (seen.count(e.edgeId)) continue;
                seen.insert(e.edgeId);
                json entry;
                entry["blocked"] = e.blocked;
                entry["blockReason"] = e.blockReason;
                entry["trafficLevel"] = e.trafficLevel;
                entry["real"] = e.real;
                if (e.real) {
                    entry["distanceKm"] = e.distanceKm;
                    entry["weight"] = e.weight;
                    json wp = json::array();
                    for (auto& [lat, lng] : e.waypoints) wp.push_back({lat, lng});
                    entry["waypoints"] = wp;
                }
                edges[e.edgeId] = entry;
            }
        }
        json state; state["edges"] = edges;
        std::ofstream out(_stateFile);
        out << state.dump(2);
    }

    void applyTo(Graph& g, const json& state) {
        if (!state.contains("edges")) return;
        for (auto& [u, list] : g.adj) {
            for (auto& e : list) {
                if (!state["edges"].contains(e.edgeId)) continue;
                auto& saved = state["edges"][e.edgeId];
                e.blocked = saved.value("blocked", false);
                e.blockReason = saved.value("blockReason", "");
                e.trafficLevel = saved.value("trafficLevel", e.trafficLevel);
                if (saved.value("real", false)) {
                    e.real = true;
                    if (saved.contains("distanceKm")) e.distanceKm = saved["distanceKm"];
                    if (saved.contains("weight")) e.weight = saved["weight"];
                    if (saved.contains("waypoints")) {
                        e.waypoints.clear();
                        for (auto& wp : saved["waypoints"]) {
                            e.waypoints.push_back({wp[0].get<double>(), wp[1].get<double>()});
                        }
                    }
                }
            }
        }
    }

private:
    std::string _dataDir, _stateFile;
};
