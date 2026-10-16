#pragma once
// weatherEnrichment.hpp — port of backend/src/weatherEnrichment.js
//
// [OOP: Singleton pattern] Same guarantee as the JS version: exactly one
// WeatherCache exists per process. The constructor is private, so
// `WeatherCache w;` is a COMPILE ERROR — the only way to get an instance
// is WeatherCache::getInstance(), which is a stronger guarantee than the
// JS version's runtime throw, since C++ catches the misuse at compile
// time instead of at runtime.
#include <unordered_map>
#include <chrono>
#include <set>
#include "weatherClient.hpp"
#include "graph.hpp"

class WeatherCache {
public:
    static WeatherCache& getInstance() {
        static WeatherCache instance; // constructed once, on first use
        return instance;
    }

    // deleted copy operations: cannot accidentally duplicate the singleton
    WeatherCache(const WeatherCache&) = delete;
    void operator=(const WeatherCache&) = delete;

    std::optional<Precipitation> getRainfall(const std::string& townId, double lat, double lng, int ttlMs = 600000) {
        auto now = std::chrono::steady_clock::now();
        auto it = _cache.find(townId);
        if (it != _cache.end()) {
            auto ageMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.second).count();
            if (ageMs < ttlMs) return it->second.first;
        }
        auto data = fetchCurrentPrecipitation(lat, lng);
        if (data) _cache[townId] = {*data, now};
        else if (it != _cache.end()) return it->second.first; // stale cache beats nothing
        return data;
    }

private:
    WeatherCache() = default; // private constructor — this IS the enforcement mechanism
    std::unordered_map<std::string, std::pair<Precipitation, std::chrono::steady_clock::time_point>> _cache;
};

struct WeatherUpdate { std::string town, road, newLevel; double rainMm; };

inline std::vector<WeatherUpdate> refreshWeatherHazard(Graph& g, double heavyMm = 4.0, double moderateMm = 1.0) {
    auto& cache = WeatherCache::getInstance();
    std::vector<WeatherUpdate> updates;
    std::map<std::string,int> rank{{"low",0},{"medium",1},{"high",2}};

    for (auto& [id, node] : g.nodes) {
        auto rain = cache.getRainfall(id, node.lat, node.lng);
        if (!rain) continue;

        std::string level;
        if (rain->rainMm >= heavyMm) level = "high";
        else if (rain->rainMm >= moderateMm) level = "medium";
        if (level.empty()) continue;

        for (auto& e : g.adj[id]) {
            if (e.blocked) continue;
            if (rank[level] > rank[e.trafficLevel]) {
                g.setTraffic(id, e.to, level);
                updates.push_back({id, e.road, level, rain->rainMm});
            }
        }
    }
    return updates;
}
