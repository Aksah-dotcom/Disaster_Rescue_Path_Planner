#pragma once
// routes.hpp — port of backend/src/routes/route.js
// Registers the same endpoints on an httplib::Server that Express
// registered on an Express Router: GET/POST /api/graph, /roads/*,
// /route, /route/k-shortest, /journey/*, /route/export, /replay/*.
#include "vendor/httplib.h"
#include <nlohmann/json.hpp>
#include "graph.hpp"
#include "journey.hpp"
#include "errorHandler.hpp"
#include "routeExport.hpp"
#include "data/historicalEvents.hpp"

using json = nlohmann::json;

inline json routeMetrics(const Graph& g, const std::vector<std::string>& path) {
    double totalDistanceKm = 0, etaMinutes = 0;
    std::vector<std::string> roadNames;
    std::set<std::string> seen;
    for (size_t i = 0; i + 1 < path.size(); i++) {
        auto e = g.getEdgeInfo(path[i], path[i+1]);
        if (e) {
            totalDistanceKm += e->distanceKm;
            etaMinutes += trafficModel::travelTimeMinutes(e->distanceKm, e->trafficLevel);
            if (!seen.count(e->road)) { seen.insert(e->road); roadNames.push_back(e->road); }
        }
    }
    return {
        {"roads", roadNames},
        {"totalDistanceKm", std::round(totalDistanceKm * 10.0) / 10.0},
        {"etaMinutes", std::round(etaMinutes * 10.0) / 10.0}
    };
}

inline void sendJson(httplib::Response& res, const json& body, int status = 200) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

inline void sendError(httplib::Response& res, int status, const std::string& message) {
    sendJson(res, {{"error", message}}, status);
}

inline void registerRoutes(httplib::Server& svr, Graph& graph, JourneyManager& journeys, std::function<void()> persist) {

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        sendJson(res, {{"status", "ok"}});
    });

    svr.Get("/api/graph", [&](const httplib::Request&, httplib::Response& res) {
        sendJson(res, graph.toJSON());
    });

    svr.Post("/api/roads/block", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string a = body.at("a"), b = body.at("b");
            std::string reason = body.value("reason", "reported blocked");
            if (!graph.hasNode(a) || !graph.hasNode(b)) throw HttpError(404, "Unknown zone id");
            if (!graph.setBlocked(a, b, true, reason)) throw HttpError(404, "No road between " + a + " and " + b);
            persist();
            sendJson(res, {{"message", "Road " + a + "-" + b + " blocked"}, {"graph", graph.toJSON()}});
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::exception&) { sendError(res, 400, "a and b are required"); }
    });

    svr.Post("/api/roads/unblock", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string a = body.at("a"), b = body.at("b");
            if (!graph.hasNode(a) || !graph.hasNode(b)) throw HttpError(404, "Unknown zone id");
            if (!graph.setBlocked(a, b, false)) throw HttpError(404, "No road between " + a + " and " + b);
            persist();
            sendJson(res, {{"message", "Road " + a + "-" + b + " cleared"}, {"graph", graph.toJSON()}});
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::exception&) { sendError(res, 400, "a and b are required"); }
    });

    svr.Post("/api/route", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string start = body.at("start"), end = body.at("end");
            std::string algorithm = body.value("algorithm", "dijkstra");
            if (!graph.hasNode(start) || !graph.hasNode(end)) throw HttpError(404, "Unknown zone id");

            auto strategy = RoutingStrategy::create(algorithm); // throws std::invalid_argument if unknown
            auto result = strategy->findRoute(graph, start, end);

            json out = {
                {"start", start}, {"end", end}, {"algorithm", algorithm},
                {"found", !result.path.empty()}, {"path", result.path}, {"order", result.order},
                {"cost", result.hasCost ? json(result.cost) : json(nullptr)}
            };
            json metrics = routeMetrics(graph, result.path); // named, not a temporary
            for (auto& [k, v] : metrics.items()) out[k] = v;
            sendJson(res, out);
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::invalid_argument& e) { sendError(res, 400, e.what()); }
        catch (std::exception&) { sendError(res, 400, "start and end are required"); }
    });

    svr.Post("/api/route/k-shortest", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string start = body.at("start"), end = body.at("end");
            int k = std::min(std::max(body.value("k", 3), 1), 5);
            if (!graph.hasNode(start) || !graph.hasNode(end)) throw HttpError(404, "Unknown zone id");

            KShortestStrategy kss;
            auto routes = kss.findRoutes(graph, start, end, k);
            json routesJson = json::array();
            for (auto& r : routes) {
                json entry = {{"rank", r.rank}, {"path", r.path}, {"cost", r.cost}};
                json metrics = routeMetrics(graph, r.path); // named, not a temporary
                for (auto& [key, v] : metrics.items()) entry[key] = v;
                routesJson.push_back(entry);
            }
            sendJson(res, {{"start", start}, {"end", end}, {"k", k}, {"routes", routesJson}});
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::exception&) { sendError(res, 400, "start and end are required"); }
    });

    svr.Get("/api/route/export", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string start = req.get_param_value("start");
            std::string end = req.get_param_value("end");
            std::string algorithm = req.has_param("algorithm") ? req.get_param_value("algorithm") : "dijkstra";
            if (start.empty() || end.empty()) throw HttpError(400, "start and end are required");
            if (!graph.hasNode(start) || !graph.hasNode(end)) throw HttpError(404, "Unknown zone id");

            auto strategy = RoutingStrategy::create(algorithm);
            auto result = strategy->findRoute(graph, start, end);
            if (result.path.empty()) throw HttpError(422, "No route exists between those points to export");

            std::vector<std::string> pathLabels, roadNames;
            for (auto& id : result.path) pathLabels.push_back(graph.nodes[id].label);
            for (size_t i = 0; i + 1 < result.path.size(); i++) {
                auto e = graph.getEdgeInfo(result.path[i], result.path[i+1]);
                roadNames.push_back(e ? e->road : "");
            }
            auto metrics = routeMetrics(graph, result.path);
            std::string pdf = buildRoutePdf(
                graph.nodes[start].label, graph.nodes[end].label, algorithm,
                pathLabels, roadNames,
                metrics["etaMinutes"], metrics["totalDistanceKm"],
                result.cost, result.hasCost
            );
            res.set_header("Content-Disposition", "attachment; filename=\"route-" + start + "-to-" + end + ".pdf\"");
            res.set_content(pdf, "application/pdf");
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::exception& e) { sendError(res, 400, e.what()); }
    });

    svr.Post("/api/journey", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string start = body.at("start"), destination = body.at("destination");
            std::string algorithm = body.value("algorithm", "dijkstra");
            if (!graph.hasNode(start) || !graph.hasNode(destination)) throw HttpError(404, "Unknown zone id");

            auto journey = journeys.start(graph, start, destination, algorithm);
            if (!journey) throw HttpError(422, "No route currently exists between those points");
            sendJson(res, {
                {"id", journey->id}, {"path", journey->fullPath()}, {"currentNode", journey->currentNode()},
                {"remainingPath", journey->remainingPath()}, {"isComplete", journey->isComplete()},
                {"rerouteHistory", json::array()}
            }, 201);
        } catch (HttpError& e) { sendError(res, e.status, e.what()); }
        catch (std::exception&) { sendError(res, 400, "start and destination are required"); }
    });

    auto journeyToJson = [](std::shared_ptr<Journey> j) {
        json history = json::array();
        for (auto& ev : j->rerouteHistory()) history.push_back({{"reason", ev.reason}, {"from", ev.from}, {"found", ev.found}});
        return json{
            {"id", j->id}, {"path", j->fullPath()}, {"currentNode", j->currentNode()},
            {"remainingPath", j->remainingPath()}, {"isComplete", j->isComplete()}, {"rerouteHistory", history}
        };
    };

    svr.Get(R"(/api/journey/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        auto j = journeys.get(req.matches[1]);
        if (!j) { sendError(res, 404, "Unknown journey id"); return; }
        sendJson(res, journeyToJson(j));
    });

    svr.Post(R"(/api/journey/(.+)/advance)", [&](const httplib::Request& req, httplib::Response& res) {
        auto j = journeys.get(req.matches[1]);
        if (!j) { sendError(res, 404, "Unknown journey id"); return; }
        j->advance();
        sendJson(res, journeyToJson(j));
    });

    svr.Delete(R"(/api/journey/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        if (!journeys.end(req.matches[1])) { sendError(res, 404, "Unknown journey id"); return; }
        res.status = 204;
    });

    svr.Get("/api/replay/scenarios", [](const httplib::Request&, httplib::Response& res) {
        auto scenarios = buildScenarios();
        json out = json::array();
        for (auto& [key, s] : scenarios)
            out.push_back({{"key", s.key}, {"label", s.label}, {"description", s.description}, {"eventCount", s.events.size()}});
        sendJson(res, out);
    });

    svr.Get(R"(/api/replay/(.+))", [](const httplib::Request& req, httplib::Response& res) {
        auto scenarios = buildScenarios();
        auto it = scenarios.find(req.matches[1]);
        if (it == scenarios.end()) { sendError(res, 404, "Unknown scenario"); return; }
        json events = json::array();
        for (auto& ev : it->second.events) {
            json e = {{"hour", ev.hour}, {"type", ev.type}, {"note", ev.note}};
            if (!ev.a.empty()) { e["a"] = ev.a; e["b"] = ev.b; e["reason"] = ev.reason; }
            if (!ev.level.empty()) {
                json roads = json::array();
                for (auto& r : ev.roads) roads.push_back({r.a, r.b});
                e["level"] = ev.level; e["roads"] = roads;
            }
            events.push_back(e);
        }
        sendJson(res, {{"key", it->second.key}, {"label", it->second.label}, {"description", it->second.description}, {"events", events}});
    });
}
