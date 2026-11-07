#pragma once

// trafficSimulator.hpp
// Deeksha's contribution: Improved Traffic Simulation
// Features: Random Engine, Thread Management and Safe Shutdown

#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <random>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <utility>

#include "graph.hpp"
#include "trafficModel.hpp"

class TrafficSimulator {

public:

    TrafficSimulator(
        Graph& g,
        int intervalMs,
        double changeProbability,
        std::function<void()> onChange
    )
        : _graph(g),
          _intervalMs(std::max(1, intervalMs)),
          _changeProbability(
              std::max(0.0, std::min(1.0, changeProbability))
          ),
          _onChange(std::move(onChange)) {}

    TrafficSimulator(const TrafficSimulator&) = delete;
    TrafficSimulator& operator=(const TrafficSimulator&) = delete;

    void start() {

        bool expected = false;

        if (!_running.compare_exchange_strong(expected, true))
            return;

        try {
            _thread = std::thread([this] {
                run();
            });
        }
        catch (...) {
            _running = false;
            throw;
        }
    }

    void stop() {

        _running = false;

        _cv.notify_all();

        if (_thread.joinable())
            _thread.join();
    }

    ~TrafficSimulator() {
        stop();
    }

private:

    void run() {

        std::mt19937 generator(std::random_device{}());

        std::bernoulli_distribution change(_changeProbability);

        while (_running) {

            {
                std::unique_lock<std::mutex> lock(_mutex);

                _cv.wait_for(
                    lock,
                    std::chrono::milliseconds(_intervalMs),
                    [this] {
                        return !_running.load();
                    }
                );
            }

            if (!_running)
                break;

            bool mutated = false;

            auto edges = _graph.allUnblockedEdgeEndpoints();

            for (auto& [a, b, edgeId] : edges) {

                if (!change(generator))
                    continue;

                _graph.setTraffic(
                    a,
                    b,
                    trafficModel::randomLevel()
                );

                mutated = true;
            }

            if (mutated && _onChange)
                _onChange();
        }
    }

    Graph& _graph;

    int _intervalMs;

    double _changeProbability;

    std::function<void()> _onChange;

    std::atomic<bool> _running{false};

    std::thread _thread;

    std::mutex _mutex;

    std::condition_variable _cv;
};