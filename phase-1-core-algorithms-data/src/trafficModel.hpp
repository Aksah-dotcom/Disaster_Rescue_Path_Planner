#pragma once
// trafficModel.hpp — direct port of backend/src/trafficModel.js
#include <string>
#include <cmath>
#include <cstdlib>
#include <map>

namespace trafficModel {

constexpr double BASE_SPEED_KMH = 42.0;

inline double speedMultiplier(const std::string& level) {
    static const std::map<std::string, double> MULT = {{"low", 1.0}, {"medium", 0.55}, {"high", 0.28}};
    auto it = MULT.find(level);
    return it != MULT.end() ? it->second : 1.0;
}

inline double travelTimeMinutes(double distanceKm, const std::string& trafficLevel) {
    double effectiveSpeed = BASE_SPEED_KMH * speedMultiplier(trafficLevel);
    return (distanceKm / effectiveSpeed) * 60.0;
}

inline std::string randomLevel() {
    double r = static_cast<double>(std::rand()) / RAND_MAX;
    if (r < 0.5) return "low";
    if (r < 0.82) return "medium";
    return "high";
}

} // namespace trafficModel
