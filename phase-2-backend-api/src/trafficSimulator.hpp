#pragma once
// trafficSimulator.hpp — port of backend/src/trafficSimulator.js
//
// JS used setInterval on Node's single-threaded event loop. C++ has no
// event loop by default, so this spins up a real background thread that
// sleeps and wakes on the same interval — functionally equivalent,
// mechanically different (genuine OS-level concurrency vs. cooperative
// single-threaded scheduling).
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include "graph.hpp"
#include "trafficModel.hpp"

class TrafficSimulator {
public:
    TrafficSimulator(Graph& g, int intervalMs, double changeProbability, std::function<void()> onChange)
        : _graph(g), _intervalMs(intervalMs), _changeProbability(changeProbability), _onChange(std::move(onChange)) {}

    void start() {
        _running = true;
        _thread = std::thread([this] { run(); });
    }

    void stop() {
        _running = false;
        if (_thread.joinable()) _thread.join();
    }

    ~TrafficSimulator() { stop(); }

private:
    void run() {
        while (_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(_intervalMs));
            if (!_running) break;

            bool mutated = false;
            for (auto& [a, b, edgeId] : _graph.allUnblockedEdgeEndpoints()) {
                double r = static_cast<double>(std::rand()) / RAND_MAX;
                if (r > _changeProbability) continue;
                _graph.setTraffic(a, b, trafficModel::randomLevel());
                mutated = true;
            }
            if (mutated && _onChange) _onChange();
        }
    }

    Graph& _graph;
    int _intervalMs;
    double _changeProbability;
    std::function<void()> _onChange;
    std::atomic<bool> _running{false};
    std::thread _thread;
};
