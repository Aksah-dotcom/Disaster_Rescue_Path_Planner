// client.cpp
//
// WHY THIS FILE EXISTS: the browser frontend (public/index.html,
// style.css, api.js, app.js) genuinely CANNOT be "converted to C++" —
// browsers execute JavaScript, not C++, full stop. There is no
// mechanical translation that makes sense here; the only honest
// C++ equivalent of "a client that talks to the server and lets a
// person interact with it" is a console client, which is what this is.
//
// It calls the exact same HTTP API server.cpp exposes — /api/graph,
// /api/route, /api/roads/block, /api/journey, /api/route/export — the
// same way app.js's fetch() calls do, just from a terminal instead of
// a map. It is a genuine, working client, not a placeholder.
//
// Compile: g++ -std=c++17 -O2 client.cpp -lcurl -o client
// Run    : ./client   (with server.cpp already running on localhost:4000)
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>
#include "httpClient.hpp"

using json = nlohmann::json;

std::string BASE_URL = "http://localhost:4000";

std::optional<std::string> httpPost(const std::string& path, const json& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;
    std::string response, payload = body.dump();
    std::string url = BASE_URL + path;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpClient::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return std::nullopt;
    return response;
}

void printPath(const json& j) {
    if (!j.value("found", false)) { std::cout << "  No route found.\n"; return; }
    std::cout << "  Route: ";
    for (auto& n : j["path"]) std::cout << n.get<std::string>() << " -> ";
    std::cout << "\b\b\b   \n";
    std::cout << "  ETA: " << j.value("etaMinutes", 0.0) << " min   Distance: " << j.value("totalDistanceKm", 0.0) << " km";
    if (!j["cost"].is_null()) std::cout << "   Risk score: " << j["cost"];
    std::cout << "\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << " DISASTER RESCUE PATH PLANNER — C++ CLIENT\n";
    std::cout << " (console equivalent of the browser UI)\n";
    std::cout << "=========================================\n";

    // confirm the server is actually reachable before doing anything else
    auto health = httpClient::get(BASE_URL + "/health", 3000);
    if (!health) {
        std::cout << "Could not reach server at " << BASE_URL << " — is server.cpp running?\n";
        std::cout << "Start it first: ./server\n";
        return 1;
    }
    std::cout << "Connected to " << BASE_URL << "\n";

    int choice = -1;
    while (choice != 0) {
        std::cout << "\n--------------------------------------------------\n";
        std::cout << "1. Plan a route (BFS/DFS/Dijkstra)\n";
        std::cout << "2. K-shortest paths (top 3)\n";
        std::cout << "3. Block a road\n";
        std::cout << "4. Unblock a road\n";
        std::cout << "5. Start a journey (dynamic re-routing)\n";
        std::cout << "6. Advance active journey\n";
        std::cout << "7. Export current route plan to PDF\n";
        std::cout << "0. Exit\n";
        std::cout << "Choice: ";
        if (!(std::cin >> choice)) break;

        if (choice == 1) {
            std::string s, e, algo;
            std::cout << "Start (e.g. DEH): "; std::cin >> s;
            std::cout << "End (e.g. BAD): "; std::cin >> e;
            std::cout << "Algorithm (bfs/dfs/dijkstra): "; std::cin >> algo;
            auto res = httpPost("/api/route", {{"start", s}, {"end", e}, {"algorithm", algo}});
            if (res) printPath(json::parse(*res));
            else std::cout << "  Request failed.\n";
        } else if (choice == 2) {
            std::string s, e;
            std::cout << "Start: "; std::cin >> s;
            std::cout << "End: "; std::cin >> e;
            auto res = httpPost("/api/route/k-shortest", {{"start", s}, {"end", e}, {"k", 3}});
            if (res) {
                auto j = json::parse(*res);
                for (auto& r : j["routes"]) {
                    std::cout << "  #" << r["rank"] << " cost=" << r["cost"] << " : ";
                    for (auto& n : r["path"]) std::cout << n.get<std::string>() << " ";
                    std::cout << "\n";
                }
            }
        } else if (choice == 3 || choice == 4) {
            std::string a, b;
            std::cout << "Zone A: "; std::cin >> a;
            std::cout << "Zone B: "; std::cin >> b;
            auto res = httpPost(choice == 3 ? "/api/roads/block" : "/api/roads/unblock", {{"a", a}, {"b", b}, {"reason", "reported via console client"}});
            std::cout << (res ? "  Done.\n" : "  Request failed.\n");
        } else if (choice == 5) {
            std::string s, e, algo;
            std::cout << "Start: "; std::cin >> s;
            std::cout << "Destination: "; std::cin >> e;
            std::cout << "Algorithm: "; std::cin >> algo;
            auto res = httpPost("/api/journey", {{"start", s}, {"destination", e}, {"algorithm", algo}});
            if (res) {
                auto j = json::parse(*res);
                std::cout << "  Journey id: " << j["id"].get<std::string>() << "\n  Path: ";
                for (auto& n : j["path"]) std::cout << n.get<std::string>() << " ";
                std::cout << "\n  (save this id to advance it later)\n";
            }
        } else if (choice == 6) {
            std::string id;
            std::cout << "Journey id: "; std::cin >> id;
            CURL* curl = curl_easy_init();
            std::string response, url = BASE_URL + "/api/journey/" + id + "/advance";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpClient::writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            auto j = json::parse(response);
            std::cout << "  Team now at: " << j["currentNode"] << "\n  Remaining: ";
            for (auto& n : j["remainingPath"]) std::cout << n.get<std::string>() << " ";
            std::cout << "\n";
            if (!j["rerouteHistory"].empty())
                std::cout << "  Rerouted " << j["rerouteHistory"].size() << "x\n";
        } else if (choice == 7) {
            std::string s, e, algo, filename;
            std::cout << "Start: "; std::cin >> s;
            std::cout << "End: "; std::cin >> e;
            std::cout << "Algorithm: "; std::cin >> algo;
            std::cout << "Save as (e.g. route.pdf): "; std::cin >> filename;

            CURL* curl = curl_easy_init();
            std::string response;
            std::string url = BASE_URL + "/api/route/export?start=" + s + "&end=" + e + "&algorithm=" + algo;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpClient::writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            std::ofstream out(filename, std::ios::binary);
            out << response;
            std::cout << "  Saved to " << filename << " (" << response.size() << " bytes)\n";
        }
    }
    std::cout << "Stay safe.\n";
    return 0;
}
