#pragma once
// osrmClient.hpp — port of backend/src/osrmClient.js
// Same public API, same behavior contract: returns std::nullopt on any
// failure so route enrichment can degrade gracefully, exactly as before.
#include<utility>
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "httpClient.hpp"
#include "graph.hpp"

using json = nlohmann::json;

struct OsrmResult {
    std::vector<std::pair<double,double>> geometry; // (lat, lng) pairs, Leaflet order
    double distanceKm;
    double durationMin;
};

inline std::string osrmBaseUrl() {
    const char* env = std::getenv("OSRM_BASE_URL");

    return env ? env : "https://router.project-osrm.org";
}

inline std::optional<OsrmResult> fetchRealRoute(
    const Node& from,
    const Node& to
) {
    if (!std::isfinite(from.lat) || !std::isfinite(from.lng) ||
        !std::isfinite(to.lat) || !std::isfinite(to.lng) ||
        std::abs(from.lat) > 90 || std::abs(to.lat) > 90 ||
        std::abs(from.lng) > 180 || std::abs(to.lng) > 180) {
        return std::nullopt;
    }
    std::ostringstream url;

    url << osrmBaseUrl() << "/route/v1/driving/"
        << from.lng << "," << from.lat << ";"
        << to.lng << "," << to.lat
        << "?overview=full&geometries=geojson";

    auto body = httpClient::get(url.str());

    if (!body) {
        return std::nullopt;
    }

    try {
        auto data = json::parse(*body);

        if (!data.contains("routes") ||
            !data["routes"].is_array() ||
            data["routes"].empty()) {
            return std::nullopt;
        }

        const auto& route = data["routes"][0];

        if (!route.is_object() ||
            !route.contains("distance") ||
            !route.contains("duration") ||
            !route.contains("geometry") ||
            !route["geometry"].is_object() ||
            !route["geometry"].contains("coordinates") ||
            !route["geometry"]["coordinates"].is_array()) {
            return std::nullopt;
        }

        OsrmResult result;

        result.distanceKm =
            std::round((route["distance"].get<double>() / 1000.0) * 10.0) / 10.0;

        result.durationMin =
            std::round((route["duration"].get<double>() / 60.0) * 10.0) / 10.0;

        for (auto& coord : route["geometry"]["coordinates"]) {
            if (!coord.is_array() || coord.size() < 2) {
                return std::nullopt;
            }

            // GeoJSON is [lng, lat]; Leaflet/our graph wants (lat, lng)
            result.geometry.push_back({double(coord[1]), double(coord[0])});
        }
        if (result.geometry.empty()) {
            return std::nullopt;
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}
