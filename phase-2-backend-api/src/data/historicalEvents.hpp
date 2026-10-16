#pragma once
// data/historicalEvents.hpp — port of backend/src/data/historicalEvents.js
//
// Defines disaster replay scenarios used by the /api/replay/* endpoints.
// Each scenario is a sequence of timestamped events that the frontend
// replays step-by-step to show how road conditions evolved during a
// real or realistic disaster.

#include <string>
#include <vector>
#include <map>

// A single road pair referenced in a traffic-change event
struct RoadPair { std::string a, b; };

// One event in a replay timeline
struct ReplayEvent {
    int         hour;     // hours since disaster start
    std::string type;     // "block" | "unblock" | "traffic" | "info"
    std::string note;     // human-readable description shown in the UI

    // For type == "block" or "unblock"
    std::string a, b, reason;

    // For type == "traffic"
    std::string          level;  // "low" | "medium" | "high"
    std::vector<RoadPair> roads;
};

// A full replay scenario
struct Scenario {
    std::string              key;
    std::string              label;
    std::string              description;
    std::vector<ReplayEvent> events;
};

// ── Scenario builder ────────────────────────────────────────────────────────

inline std::map<std::string, Scenario> buildScenarios() {
    std::map<std::string, Scenario> out;

    // ── 1. Kedarnath Floods — June 2013 ─────────────────────────────────────
    {
        Scenario s;
        s.key         = "kedarnath-2013";
        s.label       = "Kedarnath Floods (June 2013)";
        s.description =
            "Catastrophic cloudburst and glacial lake outburst caused flash floods "
            "along the Mandakini river. Thousands were stranded in Kedarnath, "
            "Gaurikund and Rambara. Multiple NH-58 segments washed out.";

        s.events.push_back({0,  "info",    "Cloudburst begins over Kedarnath valley — heavy rain alert issued.", "", "", ""});
        s.events.push_back({2,  "block",   "Gaurikund approach road submerged; Kedarnath access cut off.",
                            "RUD", "KED", "flash flood — road submerged"});
        s.events.push_back({4,  "traffic", "NH-58 Rishikesh to Rudraprayag gridlocked with evacuation traffic.",
                            "", "", "", "high", {{"RKS","PAU"},{"PAU","SRN"},{"SRN","RUD"}}});
        s.events.push_back({6,  "block",   "Srinagar–Rudraprayag bridge damaged by debris flow.",
                            "SRN", "RUD", "bridge undermined by debris"});
        s.events.push_back({8,  "block",   "Joshimath–Badrinath road blocked by landslide near Pandukeshwar.",
                            "JOS", "BAD", "landslide — road buried"});
        s.events.push_back({12, "info",    "Army helicopters begin airlifting survivors from Kedarnath helipad."});
        s.events.push_back({18, "unblock", "Srinagar–Rudraprayag: temporary bailey bridge installed.",
                            "SRN", "RUD", ""});
        s.events.push_back({24, "traffic", "Evacuation convoy traffic easing on NH-58 lower reaches.",
                            "", "", "", "medium", {{"RKS","PAU"},{"PAU","SRN"}}});
        s.events.push_back({48, "unblock", "Joshimath–Badrinath cleared by NDRF bulldozers.",
                            "JOS", "BAD", ""});
        s.events.push_back({72, "info",    "Kedarnath shrine road remains blocked; aerial route only for rescue teams."});

        out[s.key] = s;
    }

    // ── 2. Chamoli Disaster — February 2021 ─────────────────────────────────
    {
        Scenario s;
        s.key         = "chamoli-2021";
        s.label       = "Chamoli Glacier Burst (February 2021)";
        s.description =
            "A chunk of Nanda Devi glacier broke off, triggering a catastrophic "
            "flood wave down the Rishiganga and Dhauliganga rivers. The Tapovan "
            "hydro project tunnel was inundated; 200+ workers missing.";

        s.events.push_back({0,  "info",    "Glacier avalanche detected — Dhauliganga and Rishiganga surge warning."});
        s.events.push_back({1,  "block",   "Joshimath–Badrinath NH-58 cut by debris and flood surge.",
                            "JOS", "BAD", "glacier flood — road destroyed"});
        s.events.push_back({1,  "block",   "Chamoli–Joshimath section buried under debris.",
                            "CHA", "JOS", "debris flow from Rishiganga"});
        s.events.push_back({2,  "traffic", "All Garhwal feeder roads at maximum load with rescue vehicles.",
                            "", "", "", "high", {{"RKS","PAU"},{"SRN","RUD"},{"RUD","CHA"}}});
        s.events.push_back({3,  "info",    "SDRF and NDRF teams deployed from Dehradun and Rishikesh."});
        s.events.push_back({6,  "block",   "Rudraprayag–Chamoli road closed due to rockfall risk.",
                            "RUD", "CHA", "rockfall hazard"});
        s.events.push_back({12, "info",    "Tunnelling operation begins to reach trapped workers at Tapovan."});
        s.events.push_back({24, "unblock", "Rudraprayag–Chamoli reopened to rescue convoys only.",
                            "RUD", "CHA", ""});
        s.events.push_back({36, "traffic", "Rescue convoys moving; civilian traffic still restricted.",
                            "", "", "", "medium", {{"RUD","CHA"}}});
        s.events.push_back({96, "unblock", "Chamoli–Joshimath partially cleared; single-lane movement.",
                            "CHA", "JOS", ""});
        s.events.push_back({168,"info",    "Search operation scaled down; NH-58 restoration works ongoing."});

        out[s.key] = s;
    }

    // ── 3. Kumaon Landslides — August 2022 ──────────────────────────────────
    {
        Scenario s;
        s.key         = "kumaon-2022";
        s.label       = "Kumaon Landslides (August 2022)";
        s.description =
            "Sustained monsoon rainfall triggered multiple simultaneous landslides "
            "across Pithoragarh, Bageshwar, and Almora districts. Several villages "
            "isolated; NH-9 and SH-37 blocked for days.";

        s.events.push_back({0,  "info",    "IMD red alert issued for Pithoragarh and Bageshwar — extreme rainfall."});
        s.events.push_back({2,  "block",   "Pithoragarh–Munsiyari road blocked by massive landslide near Birthi.",
                            "PIT", "MUS", "landslide — 50 m road buried"});
        s.events.push_back({3,  "block",   "Almora–Pithoragarh NH-9 blocked at Kwarikhal.",
                            "ALM", "PIT", "landslide — debris on carriageway"});
        s.events.push_back({4,  "block",   "Bageshwar–Pithoragarh road closed — bridge approach washed away.",
                            "PIT", "BAG", "flood — bridge approach gone"});
        s.events.push_back({4,  "traffic", "Haldwani–Almora and Haldwani–Nainital roads packed with relief vehicles.",
                            "", "", "", "high", {{"HAL","NTL"},{"NTL","ALM"}}});
        s.events.push_back({8,  "info",    "SDRF teams airlifted to Munsiyari; 12 villages cut off."});
        s.events.push_back({12, "unblock", "Almora–Pithoragarh NH-9 cleared by BRO; single-lane.",
                            "ALM", "PIT", ""});
        s.events.push_back({18, "block",   "SH-37 Almora–Dwarahat blocked by fresh slide.",
                            "ALM", "DWA", "fresh landslide"});
        s.events.push_back({24, "unblock", "Bageshwar–Pithoragarh bridge temporary span installed.",
                            "PIT", "BAG", ""});
        s.events.push_back({36, "traffic", "Relief operations normalising; roads reopening progressively.",
                            "", "", "", "medium", {{"HAL","NTL"},{"NTL","ALM"},{"ALM","PIT"}}});
        s.events.push_back({48, "unblock", "Pithoragarh–Munsiyari cleared; road open with caution.",
                            "PIT", "MUS", ""});
        s.events.push_back({60, "unblock", "Almora–Dwarahat SH-37 reopened to light traffic.",
                            "ALM", "DWA", ""});
        s.events.push_back({72, "info",    "District administration lifts road closure orders; normalcy restored."});

        out[s.key] = s;
    }

    // ── 4. Uttarkashi Flash Flood — September 2023 ──────────────────────────
    {
        Scenario s;
        s.key         = "uttarkashi-2023";
        s.label       = "Uttarkashi Flash Flood (September 2023)";
        s.description =
            "Sudden cloudburst over Bhatwari block caused the Bhagirathi river to "
            "surge, washing away parts of NH-108 and isolating Gangotri pilgrims "
            "ahead of temple closing ceremonies.";

        s.events.push_back({0,  "info",    "Cloudburst over Bhatwari — Bhagirathi river rising rapidly."});
        s.events.push_back({1,  "block",   "Uttarkashi–Gangotri NH-108 blocked at Maneri by debris and high water.",
                            "UTK", "GAN", "cloudburst — road submerged"});
        s.events.push_back({2,  "traffic", "Uttarkashi town access roads congested with stranded pilgrims' vehicles.",
                            "", "", "", "high", {{"DEH","UTK"}}});
        s.events.push_back({3,  "info",    "Approx. 3,000 pilgrims stranded between Uttarkashi and Gangotri."});
        s.events.push_back({6,  "info",    "Helicopter sortie from Dehradun begins airlifting stranded pilgrims."});
        s.events.push_back({10, "unblock", "NH-108 partially cleared; convoy movement under escort.",
                            "UTK", "GAN", ""});
        s.events.push_back({14, "traffic", "Controlled convoy traffic on NH-108 resumes.",
                            "", "", "", "medium", {{"UTK","GAN"}}});
        s.events.push_back({24, "info",    "All stranded pilgrims evacuated; road reopened to normal traffic."});

        out[s.key] = s;
    }

    return out;
}
