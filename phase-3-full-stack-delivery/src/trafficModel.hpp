```cpp
#pragma once

// trafficModel.hpp
// Deeksha's contribution: Improved Traffic Model
// Features: Speed Calculation, Travel Time, Traffic Classification

#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <stdexcept>

namespace trafficModel {

constexpr double BASE_SPEED_KMH = 42.0;

// Calculate speed multiplier according to traffic
inline double speedMultiplier(const std::string& level) {

    if (level == "low")
        return 1.0;

    if (level == "medium")
        return 0.55;

    if (level == "high")
        return 0.28;

    return 1.0;
}

// Calculate effective vehicle speed
inline double effectiveSpeed(const std::string& level) {

    return BASE_SPEED_KMH * speedMultiplier(level);
}

// Calculate travel time in minutes
inline double travelTimeMinutes(
    double distanceKm,
    const std::string& trafficLevel
) {

    if (distanceKm < 0)
        throw std::invalid_argument("Distance cannot be negative");

    if (distanceKm == 0)
        return 0.0;

    double speed = effectiveSpeed(trafficLevel);

    return (distanceKm / speed) * 60.0;
}

// Calculate additional travel delay
inline double trafficDelayMinutes(
    double distanceKm,
    const std::string& trafficLevel
) {

    double actualTime = travelTimeMinutes(
        distanceKm,
        trafficLevel
    );

    double normalTime = travelTimeMinutes(
        distanceKm,
        "low"
    );

    return std::max(0.0, actualTime - normalTime);
}

// Generate random traffic level
inline std::string randomLevel() {

    static thread_local std::mt19937 generator(
        std::random_device{}()
    );

    static thread_local std::discrete_distribution<int> distribution{
        50, 32, 18
    };

    int result = distribution(generator);

    if (result == 0)
        return "low";

    if (result == 1)
        return "medium";

    return "high";
}

// Classify traffic severity
inline int trafficSeverity(const std::string& level) {

    if (level == "low")
        return 1;

    if (level == "medium")
        return 2;

    if (level == "high")
        return 3;

    return 0;
}

// Check whether traffic is heavy
inline bool isHeavyTraffic(const std::string& level) {

    return level == "high";
}

} // namespace trafficModel
```