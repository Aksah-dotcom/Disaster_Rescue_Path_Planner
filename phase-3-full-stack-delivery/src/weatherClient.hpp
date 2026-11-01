#pragma once
// weatherClient.hpp — Live Open-Meteo Weather Client
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <optional>
#include "httpClient.hpp"

using json = nlohmann::json;

struct Precipitation { double precipitationMm, rainMm; };

struct NodeWeather {
    double tempC = 20.0;
    double precipitationMm = 0.0;
    double rainMm = 0.0;
    int weatherCode = 0;
    std::string condition = "Clear sky";
    std::string icon = "☀️";
};

inline std::pair<std::string, std::string> decodeWeatherCode(int code) {
    if (code == 0) return {"Clear sky", "☀️"};
    if (code == 1) return {"Mainly clear", "🌤️"};
    if (code == 2) return {"Partly cloudy", "⛅"};
    if (code == 3) return {"Overcast", "☁️"};
    if (code == 45 || code == 48) return {"Foggy", "🌫️"};
    if (code >= 51 && code <= 55) return {"Drizzle", "🌦️"};
    if (code >= 61 && code <= 65) return {"Rain", "🌧️"};
    if (code >= 71 && code <= 77) return {"Snow", "🌨️"};
    if (code >= 80 && code <= 82) return {"Rain showers", "🌧️"};
    if (code >= 95) return {"Thunderstorm", "⛈️"};
    return {"Clear", "☀️"};
}

inline std::string weatherBaseUrl() {
    const char* env = std::getenv("WEATHER_BASE_URL");
    return env ? env : "https://api.open-meteo.com/v1/forecast";
}

inline std::optional<NodeWeather> fetchCurrentWeather(double lat, double lng) {
    std::ostringstream url;
    url << weatherBaseUrl() << "?latitude=" << lat << "&longitude=" << lng
        << "&current=temperature_2m,precipitation,rain,weather_code";

    auto body = httpClient::get(url.str(), 5000);
    if (!body) return std::nullopt;
    try {
        auto data = json::parse(*body);
        if (!data.contains("current")) return std::nullopt;
        auto& c = data["current"];
        int code = c.value("weather_code", 0);
        auto [cond, icon] = decodeWeatherCode(code);
        return NodeWeather{
            c.value("temperature_2m", 20.0),
            c.value("precipitation", 0.0),
            c.value("rain", 0.0),
            code,
            cond,
            icon
        };
    } catch (...) {
        return std::nullopt;
    }
}

inline std::optional<Precipitation> fetchCurrentPrecipitation(double lat, double lng) {
    auto w = fetchCurrentWeather(lat, lng);
    if (!w) return std::nullopt;
    return Precipitation{w->precipitationMm, w->rainMm};
}
