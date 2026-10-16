#pragma once
// weatherClient.hpp — port of backend/src/weatherClient.js
#include <nlohmann/json.hpp>
#include "httpClient.hpp"

using json = nlohmann::json;

struct Precipitation { double precipitationMm, rainMm; };

inline std::string weatherBaseUrl() {
    const char* env = std::getenv("WEATHER_BASE_URL");
    return env ? env : "https://api.open-meteo.com/v1/forecast";
}

inline std::optional<Precipitation> fetchCurrentPrecipitation(double lat, double lng) {
    std::ostringstream url;
    url << weatherBaseUrl() << "?latitude=" << lat << "&longitude=" << lng << "&current=precipitation,rain";

    auto body = httpClient::get(url.str(), 5000);
    if (!body) return std::nullopt;
    try {
        auto data = json::parse(*body);
        if (!data.contains("current")) return std::nullopt;
        auto& current = data["current"];
        return Precipitation{
            current.value("precipitation", 0.0),
            current.value("rain", 0.0)
        };
    } catch (...) {
        return std::nullopt;
    }
}
