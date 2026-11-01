#pragma once
// osrmClient.hpp — port of backend/src/osrmClient.js
// Same public API, same behavior contract: returns std::nullopt on any
// failure so route enrichment can degrade gracefully, exactly as before.
#include <nlohmann/json.hpp>
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

inline std::optional<OsrmResult> fetchRealRoute(const Node& from, const Node& to) {
    std::ostringstream url;
    url << osrmBaseUrl() << "/route/v1/driving/"
        << from.lng << "," << from.lat << ";" << to.lng << "," << to.lat
        << "?overview=full&geometries=geojson";

    auto body = httpClient::get(url.str());
    if (!body) return std::nullopt;

    try {
        auto data = json::parse(*body);
        if (!data.contains("routes") || data["routes"].empty()) return std::nullopt;
        auto& route = data["routes"][0];

        OsrmResult result;
        result.distanceKm = std::round((double(route["distance"]) / 1000.0) * 10.0) / 10.0;
        result.durationMin = std::round((double(route["duration"]) / 60.0) * 10.0) / 10.0;
        for (auto& coord : route["geometry"]["coordinates"]) {
            // GeoJSON is [lng, lat]; Leaflet/our graph wants (lat, lng)
            result.geometry.push_back({double(coord[1]), double(coord[0])});
        }
        return result;
    } catch (...) {
        return std::nullopt; // malformed JSON, unexpected shape, etc.
    }
}
