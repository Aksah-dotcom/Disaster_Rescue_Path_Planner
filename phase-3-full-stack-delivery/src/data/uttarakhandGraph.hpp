#pragma once
// data/uttarakhandGraph.hpp
//
// Builds the Uttarakhand disaster-rescue road graph used by all phases.
// Nodes are major towns/districts; edges are named highways with
// distance (km) and a risk factor (1.0 = normal, >1 = hazardous terrain).
//
// Node IDs are short codes; labels are full town names; sub is the
// district or region label shown in the frontend.

#include "../graph.hpp"

inline Graph buildUttarakhandGraph() {
    Graph g;

    // ── Nodes (id, label, sub-label, lat, lng) ────────────────────────
    // Garhwal Division
    g.addNode("DEH", "Dehradun",       "Dehradun District",    30.3165, 78.0322);
    g.addNode("HRS", "Haridwar",       "Haridwar District",    29.9457, 78.1642);
    g.addNode("RKS", "Rishikesh",      "Dehradun District",    30.0869, 78.2676);
    g.addNode("PAU", "Pauri",          "Pauri Garhwal",        30.1497, 78.7793);
    g.addNode("SRN", "Srinagar",       "Pauri Garhwal",        30.2211, 78.7794);
    g.addNode("RUD", "Rudraprayag",    "Rudraprayag District", 30.2845, 78.9817);
    g.addNode("CHA", "Chamoli",        "Chamoli District",     30.4009, 79.3244);
    g.addNode("JOS", "Joshimath",      "Chamoli District",     30.5583, 79.5649);
    g.addNode("BAD", "Badrinath",      "Chamoli District",     30.7433, 79.4938);
    g.addNode("KED", "Kedarnath",      "Rudraprayag District", 30.7346, 79.0669);
    g.addNode("UTK", "Uttarkashi",     "Uttarkashi District",  30.7268, 78.4354);
    g.addNode("GAN", "Gangotri",       "Uttarkashi District",  30.9940, 78.9354);
    g.addNode("YMU", "Yamunotri",      "Uttarkashi District",  31.0141, 78.4580);
    g.addNode("CHK", "Chakrata",       "Dehradun District",    30.7000, 77.8667);
    g.addNode("TRH", "Tehri",          "Tehri Garhwal",        30.3780, 78.4801);
    g.addNode("NEW", "New Tehri",      "Tehri Garhwal",        30.3897, 78.5302);
    g.addNode("GHT", "Ghansali",       "Tehri Garhwal",        30.4594, 78.6906);
    g.addNode("LAM", "Lansdowne",      "Pauri Garhwal",        29.8376, 78.6850);

    // Kumaon Division
    g.addNode("NTL", "Nainital",       "Nainital District",    29.3803, 79.4636);
    g.addNode("HAL", "Haldwani",       "Nainital District",    29.2183, 79.5130);
    g.addNode("ALM", "Almora",         "Almora District",      29.5971, 79.6591);
    g.addNode("PIT", "Pithoragarh",    "Pithoragarh District", 29.5830, 80.2181);
    g.addNode("BAG", "Bageshwar",      "Bageshwar District",   29.8363, 79.7726);
    g.addNode("CHP", "Champawat",      "Champawat District",   29.3345, 80.0912);
    g.addNode("RUD2","Rudrapur",       "Udham Singh Nagar",    28.9790, 79.3928);
    g.addNode("KAS", "Kashipur",       "Udham Singh Nagar",    29.2104, 78.9628);
    g.addNode("MUS", "Munsiyari",      "Pithoragarh District", 30.0670, 80.2371);
    g.addNode("DWA", "Dwarahat",       "Almora District",      29.7168, 79.4324);
    g.addNode("RAN", "Ramnagar",       "Nainital District",    29.3945, 79.1248);

    // ── Edges (a, b, road-name, distKm, riskFactor, trafficLevel) ─────
    // Risk factor: 1.0 plain, 1.3 hilly, 1.6 mountain, 2.0 high-altitude hazard

    // Dehradun spine
    g.addRoad("DEH", "HRS",  "NH-307 (Haridwar Road)",             55,  1.1, "medium");
    g.addRoad("DEH", "RKS",  "NH-7 (Dehradun-Rishikesh Direct)",    16,  1.1, "medium");
    g.addRoad("DEH", "CHK",  "Chakrata Road",                       90,  1.4, "low");
    g.addRoad("DEH", "UTK",  "NH-108 / Mussoorie",                  155, 1.5, "low");
    g.addRoad("DEH", "NEW",  "Tehri Bypass Road",                   87,  1.3, "low");

    // Rishikesh → up the Ganga valley
    g.addRoad("RKS", "HRS",  "NH-34 (Haridwar-Rishikesh)",          40,  1.1, "high");
    g.addRoad("RKS", "PAU",  "NH-7 (Rishikesh-Pauri Highway)",      90,  1.4, "medium");
    g.addRoad("RKS", "TRH",  "NH-34 / Tehri spur",                  78, 1.3, "low");

    // Pauri / Srinagar / Rudraprayag
    g.addRoad("PAU", "SRN",  "NH-7 (Pauri-Srinagar Road)",          35,  1.3, "medium");
    g.addRoad("SRN", "RUD",  "NH-7 (Srinagar-Rudraprayag)",         34,  1.4, "medium");
    g.addRoad("SRN", "TRH",  "Srinagar-Tehri Road",                 42,  1.3, "low");
    g.addRoad("RUD", "KED",  "Kedarnath Road",                      76,  2.0, "low");    // high hazard
    g.addRoad("RUD", "CHA",  "NH-7 (Rudraprayag-Chamoli)",          68,  1.5, "medium");
    g.addRoad("RUD", "GHT",  "Rudraprayag-Ghansali",                52,  1.5, "low");

    // Char Dham — Badrinath
    g.addRoad("CHA", "JOS",  "NH-7 (Chamoli-Joshimath)",            48,  1.6, "medium");
    g.addRoad("JOS", "BAD",  "NH-7 (Joshimath-Badrinath)",          46,  1.8, "low");    // high altitude

    // Tehri / New Tehri
    g.addRoad("TRH", "NEW",  "Tehri-New Tehri",    10,  1.2, "medium");
    g.addRoad("NEW", "GHT",  "NH-94",              48,  1.4, "low");

    // Uttarkashi → Char Dham (Gangotri / Yamunotri)
    g.addRoad("UTK", "GAN",  "NH-108",            100,  1.8, "low");
    g.addRoad("UTK", "YMU",  "Yamunotri Road",     82,  1.8, "low");
    g.addRoad("UTK", "TRH",  "NH-94",              72,  1.4, "low");

    // Lansdowne
    g.addRoad("LAM", "PAU",  "Kotdwar-Pauri Road", 55,  1.3, "low");
    g.addRoad("LAM", "HRS",  "NH-119",             95,  1.2, "medium");

    // Kumaon — Nainital cluster
    g.addRoad("HAL", "NTL",  "Haldwani-Nainital",  36,  1.3, "high");
    g.addRoad("HAL", "RUD2", "NH-309",             50,  1.1, "high");
    g.addRoad("HAL", "KAS",  "NH-309",             80,  1.1, "medium");
    g.addRoad("HAL", "RAN",  "Ramnagar Road",      50,  1.2, "medium");
    g.addRoad("NTL", "ALM",  "NH-87",              67,  1.3, "medium");
    g.addRoad("NTL", "DWA",  "Nainital-Dwarahat",  75,  1.4, "low");
    g.addRoad("RAN", "KAS",  "NH-309",             38,  1.1, "medium");

    // Almora cluster
    g.addRoad("ALM", "BAG",  "Bageshwar Road",     75,  1.4, "low");
    g.addRoad("ALM", "DWA",  "SH-37",              40,  1.3, "low");
    g.addRoad("ALM", "PIT",  "Pithoragarh Road",  114,  1.5, "low");
    g.addRoad("ALM", "CHP",  "Champawat Road",    120,  1.4, "low");

    // Pithoragarh / Munsiyari
    g.addRoad("PIT", "BAG",  "Bageshwar-Pithoragarh",60, 1.5, "low");
    g.addRoad("PIT", "MUS",  "Munsiyari Road",     87,  1.8, "low");
    g.addRoad("PIT", "CHP",  "NH-9",               65,  1.4, "low");
    g.addRoad("BAG", "DWA",  "Dwarahat-Bageshwar", 55,  1.4, "low");

    // Champawat
    g.addRoad("CHP", "RUD2", "Tanakpur Road",     120,  1.2, "medium");
    g.addRoad("CHP", "HAL",  "Haldwani Road",     120,  1.3, "medium");

    // Cross-division connectors (Garhwal ↔ Kumaon)
    g.addRoad("PAU", "DWA",  "Pauri-Dwarahat Road",100, 1.5, "low");
    g.addRoad("CHA", "BAG",  "Chamoli-Bageshwar",  130, 1.7, "low");

    return g;
}
