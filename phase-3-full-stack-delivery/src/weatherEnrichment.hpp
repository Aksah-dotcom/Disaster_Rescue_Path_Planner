#pragma once

// weatherEnrichment.hpp
// Deeksha's contribution: Improved Weather Cache and Hazard Detection

#include <unordered_map>
#include <chrono>
#include <set>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

#include "weatherClient.hpp"
#include "graph.hpp"

class WeatherCache {

private:
    struct CacheEntry {
        NodeWeather weather;
        std::chrono::steady_clock::time_point updatedAt;
    };

    std::unordered_map<std::string, CacheEntry> _cache;
    std::mutex cacheMutex;

    WeatherCache() = default;

public:

    static WeatherCache& getInstance() {
        static WeatherCache instance;
        return instance;
    }

    WeatherCache(const WeatherCache&) = delete;
    WeatherCache& operator=(const WeatherCache&) = delete;

    std::optional<NodeWeather> getWeather(
        const std::string& townId,
        double lat,
        double lng,
        int ttlMs = 600000
    ) {

        {
            std::lock_guard<std::mutex> lock(cacheMutex);

            auto it = _cache.find(townId);

            if (it != _cache.end()) {

                auto age = std::chrono::duration_cast<
                    std::chrono::milliseconds
                >(
                    std::chrono::steady_clock::now()
                    - it->second.updatedAt
                ).count();

                if (age < ttlMs)
                    return it->second.weather;
            }
        }

        // Fetch fresh weather outside the lock
        auto data = fetchCurrentWeather(lat, lng);

        if (data) {

            std::lock_guard<std::mutex> lock(cacheMutex);

            _cache[townId] = {
                *data,
                std::chrono::steady_clock::now()
            };

            return data;
        }

        // Return stale cached weather if API fails
        std::lock_guard<std::mutex> lock(cacheMutex);

        auto it = _cache.find(townId);

        if (it != _cache.end())
            return it->second.weather;

        return std::nullopt;
    }

    std::optional<NodeWeather> getWeatherFast(
        const std::string& townId,
        double lat,
        double lng
    ) {

        {
            std::lock_guard<std::mutex> lock(cacheMutex);

            auto it = _cache.find(townId);

            if (it != _cache.end())
                return it->second.weather;
        }

        // Estimate temperature using latitude
        double temp = 26.0 - (lat - 29.0) * 8.5;

        temp = std::max(4.0, std::min(28.0, temp));

        return NodeWeather{
            temp,
            0.0,
            0.0,
            0,
            "Clear sky",
            "Sunny"
        };
    }

    std::optional<Precipitation> getRainfall(
        const std::string& townId,
        double lat,
        double lng,
        int ttlMs = 600000
    ) {

        auto weather = getWeather(townId, lat, lng, ttlMs);

        if (!weather)
            return std::nullopt;

        return Precipitation{
            weather->precipitationMm,
            weather->rainMm
        };
    }
};


// Weather hazard update information
struct WeatherUpdate {
    std::string town;
    std::string road;
    std::string newLevel;
    double rainMm;
};


inline std::vector<WeatherUpdate> refreshWeatherHazard(
    Graph& g,
    double heavyMm = 4.0,
    double moderateMm = 1.0
) {

    auto& cache = WeatherCache::getInstance();

    std::vector<WeatherUpdate> updates;

    const std::map<std::string, int> rank = {
        {"low", 0},
        {"medium", 1},
        {"high", 2}
    };

    for (auto& [id, node] : g.nodes) {

        auto rainfall = cache.getRainfall(
            id,
            node.lat,
            node.lng
        );

        if (!rainfall)
            continue;

        std::string level;

        if (rainfall->rainMm >= heavyMm)
            level = "high";

        else if (rainfall->rainMm >= moderateMm)
            level = "medium";

        else
            continue;

        for (auto& edge : g.adj[id]) {

            if (edge.blocked)
                continue;

            auto current = rank.find(edge.trafficLevel);

            int currentRank = 0;

            if (current != rank.end())
                currentRank = current->second;

            if (rank.at(level) > currentRank) {

                g.setTraffic(id, edge.to, level);

                updates.push_back({
                    id,
                    edge.road,
                    level,
                    rainfall->rainMm
                });
            }
        }
    }

    return updates;
}